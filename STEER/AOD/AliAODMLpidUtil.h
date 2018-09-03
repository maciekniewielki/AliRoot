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
    //Number of the currently processed event (calculated before any cuts)
    int currentEventID;
    //Hashmap to store PID responses. The key is a concatenated (trackNumber, eventNumber) pair;
    std::unordered_map<long, AliMLPIDResponse*> responses;

    public:
    //Constructors/Destructors
    AliAODMLpidUtil();
    
    //Setters
    void incrementEventID() {++currentEventID;}
    void setEventID(int eventID) {currentEventID=eventID;}

    void addPIDResponse(int trackID, int eventID, AliMLPIDResponse*);
    void addPIDResponse(long key, AliMLPIDResponse* response);
    
    //Getters
    AliMLPIDResponse* getTrackPIDResponse(int trackID);
    AliMLPIDResponse* getTrackPIDResponse(int trackID, int eventID);
    AliMLPIDResponse* getTrackPIDResponse(long key);
};

#endif //ALIAODMLPIDUTIL_H
