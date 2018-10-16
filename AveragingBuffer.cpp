#include "AveragingBuffer.h"


void AveragingBuffer::Init(void)
{
    pointer = 0;
    sum[0] = 0.; sum[1] = 0.; sum[2] = 0.;
    averages_valid = false;
    max_dev_temp = 0.;
    just_triggered = false;
}


float AveragingBuffer::GetTrigger(void)
{
    return triggerLevel;
}

void AveragingBuffer::SetTrigger(float trig)
{
    triggerLevel = trig;
}

float AveragingBuffer::CalculateDeviation(float in[3])
{
    // Calculate a squared distance from the average. This could be a
    // euclidean distance, here we use maximum absolute difference
    // on any axis.
  
    float b = 0.;
    float a;
  
    // Find X-axis deviation
    if (in[0] > averages[0])
    {
        a = in[0] - averages[0];
    }
    else
    {
        a = averages[0] - in[0];
    }
    if (a > b)
    {
        b = a;
    }
  
    // Find Y-axis deviation
    if (in[1] > averages[1])
    {
        a = in[1] - averages[1];
    }
    else
    {
        a = averages[1] - in[1];
    }
    if (a > b)
    {
        b = a;
    }
  
    // Find Z-axis deviation
    if (in[2] > averages[2])
    {
        a = in[2] - averages[2];
    }
    else
    {
        a = averages[2] - in[2];
    }
    if (a > b)
    {
        b = a;
    }
  
    return b;

}



bool AveragingBuffer::IsTriggered(void)
{

    if (just_triggered)
    {
        just_triggered = false;
        return true;
    }
    return false;
}



void AveragingBuffer::Add(float data[3])
{
    if (pointer < size)
    {
        if (averages_valid)
        {
            float k = CalculateDeviation(data);
            if (k > GetTrigger())
            {
                just_triggered = true;
            }

            if (k > max_dev_temp) max_dev_temp = k;
        }
    }

    if (pointer >= size)
    {
        // Loop round
        pointer = 0;

        // Calculate averages for this last loop
        int count = size;
        averages[0] = sum[0] / count;
        averages[1] = sum[1] / count;
        averages[2] = sum[2] / count;
        max_dev = max_dev_temp;
        averages_valid = true;
        sum[0] = 0.; sum[1] = 0.; sum[2] = 0.;
        max_dev_temp = 0.;
    }

    sum[0] += data[0]; sum[1] += data[1]; sum[2] += data[2];
    pointer ++;   
  
}
