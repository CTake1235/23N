#include "mbed.h"
#include "PS3.h"
#include "HMC5883L.h"

// MDのアドレス
#define MIGI_MAE        0x52
#define HIDARI_MAE      0x51
#define MIGI_USIRO      0x50
#define HIDARI_USIRO    0x49

#define WOOD 0.5


// 車輪の前進、後退、ブレーキ、ゆっくり（角材超え）
static char  FWD = 0xe0;
static char  BCK = 0x20;
static char  BRK = 0x80;
static char  SLW = 0xc0;

PS3         ps3(A0,A1);     //PA_9,PA_10
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
AnalogIn    LC(PC_4); // 左中
AnalogIn    RB(PC_2); // 右後
AnalogIn    LB(PC_3); // 左後

void        send(char add, char dat);
void        getdata(void);
void        sensor_reader(float*);
void        autorun(float*);

// デバッグ用関数
void        debugger(float*);
bool ue,sita,migi,hidari,select,start,batu,maru,R1,L1;

int main(){
    sig = 0;
    float value[6];

    // 全エアシリをオンにする
    air1.write(1);
    air2.write(1);
    air3.write(1);
    while (true) {
        getdata();
        sensor_reader(value);
        debugger(value);
        if(select){
            sig = 1;
        }
        else if(start){
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
            autorun(value);
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

void sensor_reader(float* value){
    value[0] = RF.read();
    value[1] = LF.read();
    value[2] = RC.read();
    value[3] = LC.read();
    value[4] = RB.read();
    value[5] = LB.read();
}

void autorun(float* value){
    while(!batu){
        getdata();

        // 前方ふたつが読み取っていた場合: 前輪持ち上げ
        if(value[0] >= WOOD || value[1] >= WOOD){
            air1 = 1;
        }

        // 中間ふたつ: 前輪下げ、中輪上げ 
        else if (value[2] >= WOOD || value[3] >= WOOD){
            air1 = 0;
            air2 = 1;
        }

        // 後方ふたつ: 中輪下げ、後輪上げ下げ
        // センサー不足のため時間で対応、ここは要検討 //
        else if(value[4] >= WOOD || value[5] >= WOOD){
            air2 = 0;
            air3 = 1;
            ThisThread::sleep_for(1000ms);
            air3 = 0;
        }
        send(MIGI_MAE,      SLW);
        send(HIDARI_MAE,    SLW);
        send(MIGI_USIRO,    SLW);
        send(HIDARI_USIRO,  SLW);
    }
}

void debugger(float* value){
    // 赤外線センサーのデータ
    printf("value:\n右前: %f\t左前: %f\n右中: %f\t左中: %f\n右後: %f\t左後: %f\n",value[0],value[1],value[2],value[3],value[4],value[5]);

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
    printf("電源基板: %d\n\n",sig.read());
}