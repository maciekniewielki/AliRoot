#ifndef ALIAODMLPIDUTIL_H
#define ALIAODMLPIDUTIL_H

#include <unordered_map>

typedef struct
{
    int predictedPDG;
    std::unordered_map<int, float> probabilities;

}AliMLPIDResponse;


class AliAODMLpidUtil
{
    //Hashmap to store PID responses. The key is the uniqueID variable of the track
    std::unordered_map<int, AliMLPIDResponse*> responses;

    public:
    
    void addPIDResponse(int trackID, AliMLPIDResponse* response);
    AliMLPIDResponse* getTrackPIDResponse(int trackID);

    void clearResponses();
};

#endif //ALIAODMLPIDUTIL_H
