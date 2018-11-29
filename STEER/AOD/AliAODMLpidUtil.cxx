#include "AliAODMLpidUtil.h"


void AliAODMLpidUtil::addPIDResponse(int trackID, AliMLPIDResponse* response)
{
    this->responses[trackID] = response;
}

AliMLPIDResponse* AliAODMLpidUtil::getTrackPIDResponse(int trackID)
{
    return this->responses[trackID];
}

void AliAODMLpidUtil::clearResponses()
{
    for (auto&& p : this->responses) {delete p.second;}
    this->responses.clear();
}
