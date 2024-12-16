#include "DiskActivityMonitor.h"

// Constructor: Initializes member variables to default values.
// - `hQuery` and `hCounter` are set to `nullptr` to indicate they are not yet initialized.
// - `activityThreshold` is set to 5,000,000 bytes/sec as the default threshold.
// - `diskActivity` is initialized to 0, meaning no activity recorded yet.
DiskActivityMonitor::DiskActivityMonitor()
    : hQuery(nullptr), hCounter(nullptr), activityThreshold(10000000), diskActivity(0)
{
}

// Destructor: Ensures that the PDH query is properly closed when the object is destroyed.
// - If `hQuery` is not `nullptr`, it means a query was opened, so it is closed using `PdhCloseQuery`.
DiskActivityMonitor::~DiskActivityMonitor()
{
    if (hQuery)
    {
        PdhCloseQuery(hQuery);
    }
}

double DiskActivityMonitor::GetCurrentActivity() const
{
    return diskActivity;
}

// Initializes the disk activity monitor by setting up a PDH query and adding a counter.
// - Opens a new PDH query using `PdhOpenQuery`. If this fails, the function returns `false`.
// - Adds a counter to monitor the total disk bytes per second using `PdhAddCounter`. If this fails, the function returns `false`.
// - Returns `true` if both operations succeed.
bool DiskActivityMonitor::Initialize()
{
    if (PdhOpenQuery(NULL, 0, &hQuery) != ERROR_SUCCESS)
    {
        return false;
    }

    if (PdhAddCounter(hQuery, L"\\PhysicalDisk(_Total)\\Disk Bytes/sec", 0, &hCounter) != ERROR_SUCCESS)
    {
        return false;
    }

    return true;
}

// Updates the current disk activity by collecting data from the PDH query.
// - Calls `PdhCollectQueryData` to retrieve the latest data for the query. If this fails, the function returns `false`.
// - If data collection succeeds, retrieves the counter value in double format using `PdhGetFormattedCounterValue`.
// - Stores the retrieved value in `diskActivity` and returns `true` if successful.
bool DiskActivityMonitor::Update()
{
    if (PdhCollectQueryData(hQuery) != ERROR_SUCCESS)
    {
        return false;
    }

    PDH_FMT_COUNTERVALUE counterValue;
    if (PdhGetFormattedCounterValue(hCounter, PDH_FMT_DOUBLE, NULL, &counterValue) == ERROR_SUCCESS)
    {
        diskActivity = counterValue.doubleValue;
        return true;
    }

    return false;
}

// Checks whether the current disk activity exceeds the specified threshold.
// - Compares the value of `diskActivity` with `activityThreshold`.
// - Returns `true` if `diskActivity` is greater than `activityThreshold`, otherwise `false`.
bool DiskActivityMonitor::IsActivityAboveThreshold() const
{
    return diskActivity > activityThreshold;
}

// Sets a new threshold for disk activity monitoring.
// - Updates the value of `activityThreshold` to the specified `threshold`.
void DiskActivityMonitor::SetThreshold(double threshold)
{
    activityThreshold = threshold;
}

// Retrieves the current threshold for disk activity monitoring.
// - Returns the value of `activityThreshold`.
double DiskActivityMonitor::GetThreshold() const
{
    return activityThreshold;
}