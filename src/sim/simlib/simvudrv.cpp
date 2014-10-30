/** @file simvudrv.cpp
* Implementation of SimVuDrv.h classes
* sfr: rewrite
*/

#include <vector>

#include "SimDrive.h"
#include "stdhdr.h"
#include "simvudrv.h"
#include "simmover.h"
#include "object.h"
#include "mesg.h"
#include "airframe.h"
#include "aircrft.h"
#include "falcsess.h"

using namespace std;

// useful constants

/** factor from units bubble to send fine updates */
#define FINE_DISTANCE_FACTOR   (0.2f) // % of bubble
/** rough update distance (ft) */
#define ROUGH_DISTANCE_FACTOR  (1.0f) // bubble range

/** number of position updates sent for objects inside fine distance */
#define FINE_POSITIONAL_UPDATES_PER_SEC    (5)
#define FINE_POSITIONAL_UPDATES_PERIOD     (VU_TICS_PER_SECOND / FINE_POSITIONAL_UPDATES_PER_SEC)
/** number of seconds per rough position updates (outside fine, less than rough)*/
#define ROUGH_POSITIONAL_UPDATE_TIME       (2)
#define ROUGH_POSITIONAL_UPDATES_PERIOD    (VU_TICS_PER_SECOND * ROUGH_POSITIONAL_UPDATE_TIME)
/** number of seconds per mandatory position update (outside rough) */
#define MANDATORY_POSITIONAL_UPDATE_TIME   (10)
#define MANDATORY_POSITIONAL_UPDATE_PERIOD (VU_TICS_PER_SECOND*MANDATORY_POSITIONAL_UPDATE_TIME)

/** we predict unit position in current time + prediction period
* the higher this time, smoother updates, but errors get bigger too
*/
#define PREDICTION_PERIOD (VU_TICS_PER_SECOND*MANDATORY_POSITIONAL_UPDATE_TIME/3)


////////////////
// SpotDriver //
////////////////

SpotDriver::SpotDriver(VuEntity *entity) : VuMaster(entity)
{
    last_time = 0;
    lx = ly = lz = 0;
}

VU_BOOL SpotDriver::ExecModel(VU_TIME)
{
    return TRUE;
}

void SpotDriver::Exec(VU_TIME time)
{
    float dx, dy, dz;

    if (time - last_time > 2000)
    {
        last_time = time;

        dx = vuabs(lx - entity_->XPos());
        dy = vuabs(ly - entity_->YPos());
        dz = vuabs(lz - entity_->ZPos());

        if (dx + dy + dz < 4000)
        {
            if (dx + dy + dz < 1.0)
            {
                entity_->SetDelta(0.0, 0.0, 0.0);

            }
            else
            {
                entity_->SetDelta(0.001F, 0.001F, 0.001F);
            }

            VuMaster::Exec(time);
        }

        lx = entity_->XPos();
        ly = entity_->YPos();
        lz = entity_->ZPos();
    }
}


/////////////////
// SimVuDriver //
/////////////////

VU_BOOL SimVuDriver::ExecModel(VU_TIME timestamp)
{
    SimBaseClass *ptr = static_cast<SimBaseClass*>(entity_);
    VU_BOOL retval = (VU_BOOL)(ptr->Exec());
    // sfr: i think this is being called twice (one in vudriver, other here)
    entity_->SetUpdateTime(timestamp);
    return retval;
}

VuMaster::SEND_SCORE SimVuDriver::SendScore(const VuSessionEntity *vs, VU_TIME timeDelta)
{
    // TODO2 implement const functions at FalconSessionEntity
    FalconSessionEntity *localSession = static_cast<FalconSessionEntity*>(vuLocalSessionEntity.get());
    const FalconSessionEntity *targetSession = static_cast<const FalconSessionEntity*>(vs);
    SimBaseClass *pEntity = static_cast<SimBaseClass*>(targetSession->GetPlayerEntity());
    SimMoverClass *entity = static_cast<SimMoverClass*>(entity_);

    // invalid entity or time smaller than fine
    if (
        (timeDelta < FINE_POSITIONAL_UPDATES_PERIOD) or
        (entity->VuState() not_eq VU_MEM_ACTIVE)
        /* or ( not ToleranceReached())*/
    )
    {
        return SEND_SCORE(DONT_SEND, 0.0f);
    }

    // here we are above fine delta
    // fine checks

    // to server, always send fine and out of band
    if (targetSession->Id() == vuLocalGame->OwnerId())
    {
        return SEND_SCORE(SEND_OOB, 0.0f);
    }

    // if entity is in the same group as the player entity, fine OOB
    // for example, player in the same flight
    if (pEntity not_eq NULL)
    {
        CampBaseClass *pCampObj = pEntity->GetCampaignObject();
        CampBaseClass *eCampObj = entity->GetCampaignObject();

        if ((pCampObj not_eq NULL) and (pCampObj == eCampObj))
        {
            return SEND_SCORE(SEND_OOB, 0.0f);
        }
    }


    // if entity is in interest list of a session, send fine
    // TODO make targets interest of entities, like missile and radar stuff
#if FINE_INT

    if (targetSession->HasFineInterest(entity))
    {
        return SEND_SCORE(SEND_OOB, 0.0f);
    }

#endif

    BIG_SCALAR fineDist = entity->EntityType()->bubbleRange_ * FINE_DISTANCE_FACTOR;
    BIG_SCALAR fineDistD2 = fineDist * fineDist;

    // distance from entity to session squared
    BIG_SCALAR sessionD2;

    if (pEntity not_eq NULL)
    {
        // if entity is closer to session entity than fine update distance, send
        BIG_SCALAR x2, y2, z2;
        x2 = entity_->XPos() - pEntity->XPos();
        y2 = entity_->YPos() - pEntity->YPos();
        z2 = entity_->ZPos() - pEntity->ZPos();
        x2 *= x2;
        y2 *= y2;
        z2 *= z2;
        sessionD2 = x2 + y2 + z2;

        if (sessionD2 <= fineDistD2)
        {
            return SEND_SCORE(ENQUEUE_SEND, sessionD2);
        }
    }

    // distance from camera entities squared
    std::vector<BIG_SCALAR> cameraD2 = std::vector<BIG_SCALAR>(targetSession->CameraCount());

    // if entity is close to any of session camera, send
    for (int i = 0; i < targetSession->CameraCount(); ++i)
    {
        VuEntity *ce = targetSession->GetCameraEntity(i);

        if ((ce == NULL) or ce->IsSession())
        {
            continue;
        }

        FalconEntity *cameraEntity = static_cast<FalconEntity*>(ce);

        // session camera has entity... (like flight has a plane)
        if (cameraEntity->HasEntity(entity_))
        {
            // we are using 0 here, which means high priority...
            // should we use SEND_OOB instead?
            return SEND_SCORE(ENQUEUE_SEND, 0.0f);;
        }

        // ... or is close to it
        BIG_SCALAR x2, y2, z2;
        x2 = entity_->XPos() - cameraEntity->XPos();
        x2 *= x2;
        y2 = entity_->YPos() - cameraEntity->YPos();
        y2 *= y2;
        z2 = entity_->ZPos() - cameraEntity->ZPos();
        z2 *= z2;
        cameraD2[i] = x2 + y2 + z2;

        if (cameraD2[i] <= fineDistD2)
        {
            return SEND_SCORE(ENQUEUE_SEND, cameraD2[i]);
        }
    }

    // here we are above fine period but didnt send, check rough

    // not enough time for rough update
    if (timeDelta < ROUGH_POSITIONAL_UPDATES_PERIOD)
    {
        return SEND_SCORE(DONT_SEND, 0.0f);
    }

    // get rough distance squared
    BIG_SCALAR roughDistD2 = entity->EntityType()->bubbleRange_ * ROUGH_DISTANCE_FACTOR;
    roughDistD2 *= roughDistD2;

    // if entity is in rough range of session entity, check rough (distance is already computed)
    if ((pEntity not_eq NULL) and (sessionD2 <= roughDistD2))
    {
        return SEND_SCORE(ENQUEUE_SEND, roughDistD2);
    }

    // if entity is in rough range from any of session camera, send rough (distance already computed)
    for (unsigned int i = 0; i < cameraD2.size(); ++i)
    {
        VuEntity *ce = targetSession->GetCameraEntity(i);

        if ((ce == NULL) or ce->IsSession())
        {
            continue;
        }

        if (cameraD2[i] <= roughDistD2)
        {
            return SEND_SCORE(ENQUEUE_SEND, cameraD2[i]);
        }
    }

    // last, maximum time reached, send out of band
    if (timeDelta > MANDATORY_POSITIONAL_UPDATE_PERIOD)
    {
        return SEND_SCORE(SEND_OOB, 0.0f);
    }

    // no checks passed, dont send
    return SEND_SCORE(DONT_SEND, 0.0f);
}



////////////////
// SimVuSlave //
////////////////

void SimVuSlave::Exec(VU_TIME timestamp)
{
    // base class call. zero turn speed if interval is greater than fine period
    // avoids infinite turning in case we stop receiving updates from the unit (changed to rough for example)
    if (timestamp > ((lastRemoteUpdateTime_ + FINE_POSITIONAL_UPDATES_PERIOD)))
    {
        d_dryaw_ = d_drpitch_ = d_drroll_ = 0;
    }

    VuDelaySlave::Exec(timestamp);
    // tricles down to all the inheritance classes
    ((SimBaseClass*)entity_)->Exec();
}

VU_ERRCODE SimVuSlave::Handle(VuPositionUpdateEvent *event)
{
    VU_ERRCODE err = VU_SUCCESS;

    if ( not ((SimBaseClass*)entity_)->IsAwake())
    {
        VuDelaySlave::Handle(event);
        entity_->SetUpdateTime(vuxGameTime);
        // if we are not awake - (and also by implication remote)
        // then noexec because we are not actually in the simobjectlist
        NoExec(vuxGameTime);
    }
    else
    {
        VuSessionEntity *localSession = vuLocalSessionEntity.get();
        VuEntity *localEntity = static_cast<FalconSessionEntity*>(localSession)->GetPlayerEntity();

        if (predictedTime_ == 0)
        {
            // just set position, no predictions
            entity_->SetPosition(event->x_, event->y_, event->z_);
            entity_->SetDelta(event->dx_, event->dy_, event->dz_);
            entity_->SetYPR(event->yaw_, event->pitch_, event->roll_);
            VuDelaySlave::Reset();
            predictedTime_ = event->updateTime_;
        }
        // predict
        else
        {
            double px, py, pz;

            // past or almost present event
            if (event->updateTime_ < vuxGameTime + PREDICTION_PERIOD)
            {
                predictedTime_ = vuxGameTime + PREDICTION_PERIOD;
                double dt_ev = GetDT(predictedTime_, event->updateTime_);
                // predicted position in future
                px = (event->x_ + event->dx_ * dt_ev);
                py = (event->y_ + event->dy_ * dt_ev);
                pz = (event->z_ + event->dz_ * dt_ev);
            }
            // far future event, use update as prediction
            else
            {
                predictedTime_ = event->updateTime_;
                // position
                px = (event->x_);
                py = (event->y_);
                pz = (event->z_);
            }

            // set DR speed to reach predicted position in time
            double dt_inv = 1.0f / GetDT(predictedTime_, vuxGameTime);
            SM_SCALAR dx, dy, dz;
            dx = static_cast<SM_SCALAR>((px - drx_) * dt_inv);
            dy = static_cast<SM_SCALAR>((py - dry_) * dt_inv);
            dz = static_cast<SM_SCALAR>((pz - drz_) * dt_inv);

            // speed can never get bigger than 1.5 event speed
            SM_SCALAR drsp = (dx * dx + dy * dy + dz * dz);
            // use *2.25 here because its 1.5 squared
            SM_SCALAR evsp = (event->dx_ * event->dx_ + event->dy_ * event->dy_ + event->dz_ * event->dz_) * 2.25f;

            if (drsp > evsp)
            {
                SM_SCALAR adjFactor = sqrt(evsp / drsp);
                // adjust values so that speed becomes 1.5 event speed
                d_drx_ = adjFactor * dx;
                d_dry_ = adjFactor * dy;
                d_drz_ = adjFactor * dz;
            }
            else
            {
                // under limit, no prob
                d_drx_ = dx;
                d_dry_ = dy;
                d_drz_ = dz;
            }

            // since we dont receive turn speed, we cant predict like position
            // so compute turn speed such that we reach the received position until next update
            SM_SCALAR et[3], pdt[3], ct[3];
            ct[0] = dryaw_;
            ct[1] = drpitch_;
            ct[2] = drroll_;
            et[0] = event->yaw_;
            et[1] = event->pitch_;
            et[2] = event->roll_;

            for (unsigned int i = 0; i < 3; ++i)
            {
                // always below 2PI
                et[i] = fmod(et[i], VU_TWOPI);
                ct[i] = fmod(ct[i], VU_TWOPI);
                SM_SCALAR dist = et[i] - ct[i];

                // if turn too big, go other way around
                if (dist > VU_PI)
                {
                    dist -= VU_TWOPI;
                }
                else if (dist < -VU_PI)
                {
                    dist += VU_TWOPI;
                }

                // speed is dist / time for next update
                // time is the fine update period, in seconds, which is 1 / FINE_POSITIONAL_UPDATES_PER_SEC
                // thus dist / (1/FINE_POSITIONAL_UPDATES_PER_SEC) = dist * FINE_POSITIONAL_UPDATES_PER_SEC
                pdt[i] = dist * FINE_POSITIONAL_UPDATES_PER_SEC;
            }

            d_dryaw_ = pdt[0];
            d_drpitch_ = pdt[1];
            d_drroll_ = pdt[2];
        }

        lastRemoteUpdateTime_ = vuxGameTime;//event->updateTime_;
        err = VU_SUCCESS;
    }

    return err;
}

