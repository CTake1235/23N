#include "mbed.h"
#include "PS3.h"
#include "HMC5883L.h"

// MDのアドレス
#define MIGI_MAE        0x26
#define HIDARI_MAE      0x54
#define MIGI_USIRO      0x56
#define HIDARI_USIRO    0x50

#define WOOD 80


// 車輪の前進、後退、ブレーキ、ゆっくり（角材超え）
// const char  FWD = 0xe0;
// const char  BCK = 0x20;
const char  FWD = 0x98;
const char  BCK = 0x62;
const char  BRK = 0x80;
const char  SLW = 0x98;

double      dis = 0;
float       value[6];

Thread      th1;

PS3         ps3(D8,D2);     //PA_9,PA_10
I2C         motor(D14,D15); //PB_9, PB_8
HMC5883L    ChiJiKisensor(PB_4,PA_8);

//電源基板まわり
DigitalOut  sig(PC_12);     //緊急停止（オンオフ）
DigitalIn   led(PC_10);     //状態確認

//エアシリンダーズ
DigitalOut  air1(PA_12); // 前輪
DigitalOut  air2(PA_11); // 中輪
DigitalOut  air3(PB_12); // 後輪

//赤外線センサーズ
AnalogIn    RF(PA_6); // 右前
AnalogIn    LF(PA_7); // 左前
AnalogIn    RC(PC_5); // 右中
AnalogIn    LC(PC_4); // 左中、これメイン
AnalogIn    RB(PC_2); // 右後
AnalogIn    LB(PC_3); // 左後

void        send(char add, char dat);
void        getdata(void);
void        sensor_reader(void);
// void        autorun(void);
void        auto_run(void);

// デバッグ用関数
void        debugger(void);
bool ue,sita,migi,hidari,select,start,batu,maru,R1,L1;

int main(){
    sig = 0;

    // 全エアシリをオンにする
    air1.write(0);
    air2.write(0);
    air3.write(0);
    while (true) {
        getdata();
        sensor_reader();
        debugger();
        if(select == 1){
            sig = 1;
        }
        else if(start == 1){
            sig = 0;
        }
        else if(ue){
            send(MIGI_MAE,      FWD);
            send(HIDARI_MAE,    FWD);
            send(MIGI_USIRO,    FWD);
            send(HIDARI_USIRO,  FWD);
        }
        else if(sita){
            send(MIGI_MAE,      BCK);
            send(HIDARI_MAE,    BCK);
            send(MIGI_USIRO,    BCK);
            send(HIDARI_USIRO,  BCK);
        }
        else if(migi){
            send(MIGI_MAE,      BCK);
            send(HIDARI_MAE,    FWD);
            send(MIGI_USIRO,    FWD);
            send(HIDARI_USIRO,  BCK);
        }
        else if(hidari){
            send(MIGI_MAE,      FWD);
            send(HIDARI_MAE,    BCK);
            send(MIGI_USIRO,    BCK);
            send(HIDARI_USIRO,  FWD);
        }
        else if(R1){
            send(MIGI_MAE,      SLW);
            send(HIDARI_MAE,    FWD);
            send(MIGI_USIRO,    SLW);
            send(HIDARI_USIRO,  FWD);
        }
        else if(L1){
            send(MIGI_MAE,      FWD);
            send(HIDARI_MAE,    SLW);
            send(MIGI_USIRO,    FWD);
            send(HIDARI_USIRO,  SLW);
        }
        else if(maru){
            send(MIGI_MAE,      SLW);
            send(HIDARI_MAE,    SLW);
            send(MIGI_USIRO,    SLW);
            send(HIDARI_USIRO,  SLW);
            th1.start(auto_run);
        }
        else{
            send(MIGI_MAE,      BRK);
            send(HIDARI_MAE,    BRK);
            send(MIGI_USIRO,    BRK);
            send(HIDARI_USIRO,  BRK);
        }
    }
}
void send(char add, char dat){
    motor.start();
    motor.write(add);
    motor.write(dat);
    motor.stop();
    wait_us(50000);
}

void getdata(void){
    select  = ps3.getSELECTState();
    start   = ps3.getSTARTState();

    ue      = ps3.getButtonState(PS3::ue);
    sita    = ps3.getButtonState(PS3::sita);
    hidari  = ps3.getButtonState(PS3::hidari);
    migi    = ps3.getButtonState(PS3::migi);

    R1      = ps3.getButtonState(PS3::R1);
    L1      = ps3.getButtonState(PS3::L1);

    maru    = ps3.getButtonState(PS3::maru);
    batu    = ps3.getButtonState(PS3::batu);

}

void sensor_reader(void){
    value[0] = RF.read();
    value[1] = LF.read();
    value[2] = RC.read();
    value[3] = LC.read();
    value[4] = RB.read();
    value[5] = LB.read();

    dis = 71.463 * pow(LC.read(),-1.084);
}

// void autorun(void){
//     while(!batu){
//         getdata();

//         // 前方ふたつが読み取っていた場合: 前輪持ち上げ
//         if(value[0] >= WOOD || value[1] >= WOOD){
//             air1 = 1;
//         }

//         // 中間ふたつ: 前輪下げ、中輪上げ 
//         else if (value[2] >= WOOD || value[3] >= WOOD){
//             if(air1.read() == 1){
//                 air1.write(0);
//                 air2.write(1);      
//             }
//         }

//         // 後方ふたつ: 中輪下げ、後輪上げ下げ
//         // センサー不足のため時間で対応、ここは要検討 //
//         else if(value[4] >= WOOD || value[5] >= WOOD){
//             if(air2.read() == 1){
//                 air2 = 0;
//                 air3 = 1;
//                 ThisThread::sleep_for(1000ms);
//                 air3 = 0;
//             }
//         }
//         send(MIGI_MAE,      SLW);
//         send(HIDARI_MAE,    SLW);
//         send(MIGI_USIRO,    SLW);
//         send(HIDARI_USIRO,  SLW);
//     }
// }

void auto_run(void){
    while(!batu){
        getdata();
        printf("///\nauto_running!!\n///\n");
        if(dis <= WOOD){
            printf("エアシリ");
            air1 = 1;
            ThisThread::sleep_for(1s);
            air2 = 1;
            ThisThread::sleep_for(1s);
            air1 = 0;
            ThisThread::sleep_for(100ms);
            air3 = 1;
            ThisThread::sleep_for(1s);
            air2 = 0;
            ThisThread::sleep_for(1s);
            air3 = 0;
        }
    }
}

void debugger(void){
    // 赤外線センサーのデータ
    // printf("value:\n右前: %f\t左前: %f\n右中: %f\t左中: %f\n右後: %f\t左後: %f\n",value[0],value[1],value[2],value[3],value[4],value[5]);
    printf("value: %lf\n",dis);
    // PS3コンのデータ 
    if(select)  printf("select,");
    if(start)   printf("start,");
    if(ue)      printf("ue,");
    if(hidari)  printf("hidari,");
    if(sita)    printf("sita,");
    if(migi)    printf("migi,");
    if(R1)      printf("R1,");
    if(L1)      printf("L1,");
    if(maru)    printf("maru,");
    if(batu)    printf("batu,");
    printf("\n");

    // 地磁気センサーの値（見るだけ）
    printf("角度:\t%f\n",ChiJiKisensor.getHeadingXYDeg());

    // 電源基板
    if(led.read() == 0)printf("12V:ON\n");
    else printf("12V:OFF\n");
    printf("電源スイッチ:%d\n\n",!sig.read());
    printf("-------------------------\n\n");
}