
#ifndef ACSELECT_H
#define ACSELECT_H

BOOL RequestACSlot (Flight flight, uchar team, uchar plane_slot, uchar skill, int ac_type, int player);
void LeaveACSlot (Flight flight, uchar plane_slot);
void RequestFlightDelete (Flight flight);
void RequestTeamChange (Flight flight, int newteam);
void RequestTypeChange (Flight flight, int newtype);
void RequestCallsignChange (Flight flight, int newcallsign);
void RequestSkillChange (Flight flight, int plane_slot, int newskill);

#endif