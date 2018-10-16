#ifndef AVERAGING_BUFFER_H_
#define AVERAGING_BUFFER_H_


class AveragingBuffer
{
    private:

    int pointer;
    float sum[3];
    float max_dev_temp;
    float averages[3];
    float max_dev;
    bool averages_valid;
    bool just_triggered;  // one-shot record of triggering
    float triggerLevel = 100.;

    const int size = 500;
    
  
    public:
    
    void Add(float data[3]);
    void Init(void);
    bool IsTriggered(void);
    void SetTrigger(float);



    private:

    float GetTrigger(void);
    float CalculateDeviation(float [3]);
    
};



#endif // AVERAGING_BUFFER_H_

