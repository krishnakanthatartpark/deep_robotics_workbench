# GaugeInfo.msg
# Description: Combines gauge parameters, the latest gauge image, and the current reading.

inspection_interfaces/GaugeParam param      # Reference to the gauge configuration/parameters
sensor_msgs/Image gauge_image  # Image of the gauge
float32 gauge_reading          # Current reading of the gauge
