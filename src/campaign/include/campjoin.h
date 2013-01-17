
#ifndef CAMPJOIN_H
#define CAMPJOIN_H

extern void StartCampaignGame (int local, int game_type);

extern void StopCampaignLoad (void);

extern void CampaignPreloadSuccess (int remote_game);

extern void CampaignJoinSuccess (void);

extern void CampaignJoinFail (void);

extern void CampaignJoinKeepAlive (void);

extern void CampaignConnectionTimer (void);

#endif