使用ESP32S3 接入MiniMax文本语音大模型对话\
源代码参考B站大佬2345vor的教程https://blog.csdn.net/vor234/article/details/138620142  感谢！\
在原有功能基础上加入了\
1：smartconfig一键配网功能，能记录上次配网信息，断电能够实现重连。\
2：加了TFT屏幕显示文本（st7789v2 240x320 spi）显示还不完美，需要调试。\
3：由原本的2秒提问录音改成了5秒。\
%代码中APIKEY的部分经过处理请勿照搬，请自己申请后替换。%\
硬件部分：（根据需要自己更换接线）\
主控esp32-s3-devkitc-1 n16r8 上传时不要选择USB口，选择UART口。\
按键： GPIO7\
灯：  GPIO47\
麦克风：max9814 GPIO2 \
i2s播放:I2S_DOUT=GPIO6 ， I2S_BCLK=GPIO5  ，I2S_LRC=GPIO4  。\
TFT屏：st7789v2 240x320 请参考\lib\TFT_eSPI\User_Setup.h\
#define TFT_MOSI 11\
#define TFT_SCLK 12\
#define TFT_CS   10  \
#define TFT_DC    9  \
#define TFT_RST   8 \
#define TFT_MISO 13 
