// #include "mbed.h"
// #include "PS3.h"
// #include "HMC5883L.h"

// // MDのアドレス
// #define MIGI_MAE        0x52
// #define HIDARI_MAE      0x51
// #define MIGI_USIRO      0x50
// #define HIDARI_USIRO    0x49

// // 車輪の前進、後退、ブレーキ、ゆっくり（角材超え）
// #define FWD 0xe0
// #define BCK 0x20
// #define STB 0x80
// #define SLW 0xc0

// #define WOOD 0.5

// PS3         ps3(A0,A1);     //PA_9,PA_10
// I2C         motor(D14,D15); //PB_9, PB_8
// I2C         ChiJiKisensor(PB_4,PA_8);

// //電源基板まわり
// DigitalOut  sig(PC_12);     //緊急停止（オンオフ）
// DigitalIn   led(PC_10);     //状態確認

// //エアシリンダーズ
// DigitalOut  air1(PA_12);
// DigitalOut  air2(PA_11);
// DigitalOut  air3(PB_12);

// //赤外線センサーズ
// AnalogIn    RF(PA_6); // 右前
// AnalogIn    LF(PA_7); // 左前
// AnalogIn    RC(PC_5); // 右中
// AnalogIn    LC(PC_4); // 左中
// AnalogIn    RB(PC_2); // 右後
// AnalogIn    LB(PC_3); // 左後



// void        send(char add, char dat);
// void        getdata(void);
// void        sensor_reader(float*);
// void        autorun(float*);

// bool ue,sita,migi,hidari,select,start,batu,maru;

// int main(){
//     sig = 0;
//     float value[6];
//     while (true) {
//         getdata();
//         sensor_reader(value);
//         if(select){
//             sig = 1;
//         }if(start){
//             sig = 0;
//         }if(ue){
//             send(MIGI_MAE,      FWD);
//             send(HIDARI_MAE,    FWD);
//             send(MIGI_USIRO,    FWD);
//             send(HIDARI_USIRO,  FWD);
//         }if(sita){
//             send(MIGI_MAE,      BCK);
//             send(HIDARI_MAE,    BCK);
//             send(MIGI_USIRO,    BCK);
//             send(HIDARI_USIRO,  BCK);
//         }if(migi){
//             send(MIGI_MAE,      BCK);
//             send(HIDARI_MAE,    FWD);
//             send(MIGI_USIRO,    FWD);
//             send(HIDARI_USIRO,  BCK);
//         }if(hidari){
//             send(MIGI_MAE,      FWD);
//             send(HIDARI_MAE,    BCK);
//             send(MIGI_USIRO,    BCK);
//             send(HIDARI_USIRO,  FWD);
//         }if(maru){
//             autorun(value);
//         }else{
//             send(MIGI_MAE,      STB);
//             send(HIDARI_MAE,    STB);
//             send(MIGI_USIRO,    STB);
//             send(HIDARI_USIRO,  STB);
//         }
//     }
// }
// void send(char add, char dat){
//     motor.start();
//     motor.write(add);
//     motor.write(dat);
//     motor.stop();
//     wait_us(50000);
// }

// void getdata(void){
//     ue      = ps3.getButtonState(PS3::ue);
//     sita    = ps3.getButtonState(PS3::sita);
//     hidari  = ps3.getButtonState(PS3::hidari);
//     migi    = ps3.getButtonState(PS3::migi);

//     batu = ps3.getButtonState(PS3::batu);

//     select  = ps3.getSELECTState();
//     start   = ps3.getSTARTState();
// }

// void sensor_reader(float* value){
//     value[0] = RF.read();
//     value[1] = LF.read();
//     value[2] = RC.read();
//     value[3] = LC.read();
//     value[4] = RB.read();
//     value[5] = LB.read();
// }
// void autorun(float* value){
//     while(batu != 1){
//         getdata();
//         send(MIGI_MAE,      SLW);
//         send(HIDARI_MAE,    SLW);
//         send(MIGI_USIRO,    SLW);
//         send(HIDARI_USIRO,  SLW);
//         if(value[0] >= WOOD || value[1] >= WOOD){
//             air1 = 1;
//         }else if (value[2] >= WOOD || value[3] >= WOOD){
//             air1 = 0;
//             air2 = 1;
//         }else if(value[4] >= WOOD || value[5] >= WOOD){
//             air2 = 0;
//             air3 = 1;
//             ThisThread::sleep_for(1000ms);
//             air3 = 0;
//         }
//     }
// }