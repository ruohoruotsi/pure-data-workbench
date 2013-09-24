-------------------send_sensor.rj------------------------------------------

This scene is a utility patch to send iPhone/iPod sensor information from the device to a
developing scene on a desktop computer. 

Some sensors are not available for all devices, the following list shows what each device
supports:

iPhone 4 - Accelerometer, Gyroscope, Touch, GPS, Compass, Time (and Microphone) 

iPhone 3GS - Accelerometer, Touch, GPS, Compass, (and Microphone)   

iPhone 3G - Accelerometer, Touch, GPS, Time (and Microphone) 

iPhone 1G - Accelerometer, Touch, GPS, Time (and Microphone) 


iPod Touch 4G - Accelerometer, Gyroscope, Touch, Time (and Microphone) 

iPod Touch 3G - Accelerometer, Touch, Time

iPod Touch 2G - Accelerometer, Touch, Time

(Note: microphone is in brackets as the signal is not sent via send_sensor)

---------------------------------------------------------------------------

How to connect:

1) Determine IP address of the desktop computer and the mobile device
2) Load the get_sensor.pd patch available here - http://blog.rjdj.me/pages/pd-utilities
3) Enter the IP addresses into the relevant fields of the patch
4) Make sure both the computer and device are connected to the same network
5) Make sure the send_sensor.rj scene is running on the device
6) Click connect
7) ???
8) Profit!