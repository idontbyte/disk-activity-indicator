#ifndef DISK_ACTIVITY_MONITOR_H
#define DISK_ACTIVITY_MONITOR_H

#include <pdh.h>

class DiskActivityMonitor
{
public:
	DiskActivityMonitor();
	~DiskActivityMonitor();
	double GetCurrentActivity() const;
	bool Initialize();
	bool Update();
	bool IsActivityAboveThreshold() const;

	void SetThreshold(double threshold);
	double GetThreshold() const;

private:
	PDH_HQUERY hQuery;
	PDH_HCOUNTER hCounter;
	double activityThreshold;
	double diskActivity;
};

#endif // DISK_ACTIVITY_MONITOR_H