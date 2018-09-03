#include "AliAODMLpidUtil.h"


//Perfection
long inline combine(int i, int j){return (long) i << 32 | j;}

AliAODMLpidUtil::AliAODMLpidUtil()
{
    this->currentEventID = 0;
}

//Setters
void AliAODMLpidUtil::addPIDResponse(int trackID, int eventID, AliMLPIDResponse* response)
{
    this->addPIDResponse(combine(trackID, eventID), response);
}

void AliAODMLpidUtil::addPIDResponse(long key, AliMLPIDResponse* response)
{
    this->responses[key] = response;
}

//Getters
AliMLPIDResponse* AliAODMLpidUtil::getTrackPIDResponse(int trackID)
{
    return this->getTrackPIDResponse(trackID, this->currentEventID);
}

AliMLPIDResponse* AliAODMLpidUtil::getTrackPIDResponse(int trackID, int eventID)
{
    return getTrackPIDResponse(combine(trackID, eventID));
}

AliMLPIDResponse* AliAODMLpidUtil::getTrackPIDResponse(long key)
{
    return this->responses[key];
}

