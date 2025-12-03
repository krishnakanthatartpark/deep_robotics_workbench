# GaugeParam.msg
# Description: Defines a gauge, its parameters, thresholds, auto-detect behavior, optional group, and optional image.

string id                   # Gauge identifier, e.g. "P-101"
int16 aruco_id              # Optional AR marker ID associated with this gauge
string type                  # Gauge type, e.g. "pressure", "temperature"
string unit                  # Measurement unit, e.g. "bar", "C"

# Auto-detect behavior flags
bool auto_detect_max          # Auto-learn upper limit
bool auto_detect_min          # Auto-learn lower limit (can remain false for manual)
bool auto_detect_max_threshold
bool auto_detect_min_threshold

# Gauge limits
float32 min_value             # Minimum allowed value (if Nan auto detect)
float32 max_value             # Maximum allowed value (if NaN auto detect)

# Thresholds for alerts
float32 min_threshold         # Minimum threshold (NaN if not applicable)
float32 max_threshold         # Maximum threshold (NaN if not applicable)

# Optional grouping
string group_id               # Optional - gauge(s) that this reading should be paired/subtracted with
