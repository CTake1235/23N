#include "mbed.h"
#include "PS3.h"
#include "BNO055.h"
#include <cstdio>

// MDのアドレス
#define MIGI_MAE        0x26
#define HIDARI_MAE      0x54
#define MIGI_USIRO      0x56
#define HIDARI_USIRO    0x50

#define WOOD            90 // [mm]

// 車輪の前進、後退、ブレーキ、ゆっくり（角材超え）
// const char  FWD = 0xe0;
// const char  BCK = 0x20;
const char  FWD = 0x98 + 16;
const char  BCK = 0x62 - 16;
const char  BRK = 0x80;
const char  SLW = 0x98 + 16;



Ticker      getter;

PS3         ps3(D8,D2);     //PA_9,PA_10
I2C         motor(D14,D15); //PB_9, PB_8
BNO055      ChiJiKisensor(PB_4,PA_8);

//電源基板まわり
DigitalOut  sig(PC_12);     //緊急停止（オンオフ）
DigitalIn   led(PC_10);     //状態確認

//エアシリンダーズ
DigitalOut  air1(PA_12); // 前輪
// DigitalOut  air2(PA_11); // 中輪 使わない
DigitalOut  air3(PB_12); // 後輪

DigitalOut  myled(LED1);

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
void        stater(void);

// デバッグ用関数
void        debugger(void);
bool ue,sita,migi,hidari,select,start,batu,maru,sankaku,R1,R2,L2;
bool state;

double      dis = 0;
float       value[6];

int main(){
    sig = 1;
    state = false;
    // 全エアシリをオンにする
    // 信号が来ないとき、足回りは展開されている
    air1.write(0);
    // air2.write(0);
    air3.write(0);
    myled.write(0);

    char r_fwd = FWD;
    char r_bck = BCK;

    // 地磁気センサー初期化、見つかるまでLチカ
    ChiJiKisensor.reset();
    while(!ChiJiKisensor.check())myled.write(!myled.read());
    myled.write(1); // 見つかったら光らっせぱにしておく
    while (true) {
        getdata();
        sensor_reader();
        auto_run();
        debugger();
        if(ChiJiKisensor.euler.yaw <= 350 && ChiJiKisensor.euler.yaw >= 270){
            r_fwd = FWD + // 上げたい分
            r_bck = BCK - 
        }
        else if(ChiJiKisensor.euler.yaw <= 90 && ChiJiKisensor.euler.yaw <= 10){
            r_fwd = FWD + 
            r_bck = BCK - 
        }
        else{
            r_fwd = FWD;
            r_bck = BCK;
        }
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

        // 前エアシリ下げ
        else if(R2 && L2)       air1.write(1);

        // 中エアシリ下げ
        // else if(R1 && L2)       air2.write(1);

        // 後エアシリ下げ
        else if(sankaku && L2)  air3.write(1);

        // 前エアシリ上げ
        else if(R2)             air1.write(0);

        // 中エアシリ上げ
        // else if(R1)             air2.write(0);

        // 後エアシリ上げ
        else if(sankaku)        air3.write(0);

        // 自動角材超え開始
        else if(maru){
            state = true;
            send(MIGI_MAE,      FWD);
            send(HIDARI_MAE,    FWD);
            send(MIGI_USIRO,    FWD);
            send(HIDARI_USIRO,  FWD);
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

    R2      = ps3.getButtonState(PS3::R2);
    R1      = ps3.getButtonState(PS3::R1);
    L2      = ps3.getButtonState(PS3::L2);

    sankaku = ps3.getButtonState(PS3::sankaku);
    maru    = ps3.getButtonState(PS3::maru);
    batu    = ps3.getButtonState(PS3::batu);

}

void sensor_reader(void){

    // 測距
    value[0] = RF.read();
    value[1] = LF.read();
    value[2] = RC.read();
    value[3] = LC.read();
    value[4] = RB.read();
    value[5] = LB.read();
    dis = 71.463 * pow(LC.read(),-1.084);

    // 地磁気
    ChiJiKisensor.get_angles();

}

void auto_run(void){
    while(state){
        getter.attach(Callback<void()>(&stater),1ms);
        if(dis <= WOOD){
                printf("エアシリ\n");
                air1 = 1;
                printf("前あげ\n");
                ThisThread::sleep_for(1s);
                // air2 = 1;
                // printf("中あげ\n");
                // ThisThread::sleep_for(1s);
                air1 = 0;
                printf("前さげ\n");
                ThisThread::sleep_for(100ms);
                air3 = 1;
                printf("後あげ\n");
                ThisThread::sleep_for(1s);
                // air2 = 0;
                // printf("中さげ\n");
                // ThisThread::sleep_for(1s);
                air3 = 0;
                printf("後さげ\n");
                state = false;
                getter.detach();
        }
    }
}

void stater(void){
    getdata();
    if(batu){
        state = false;
    }
    else{
        printf("running!\n");
    }
}

void debugger(void){
    // 赤外線センサーのデータ
    // printf("value:\n右前: %f\t左前: %f\n右中: %f\t左中: %f\n右後: %f\t左後: %f\n",value[0],value[1],value[2],value[3],value[4],value[5]);
    printf("value: %lf\n",dis);

    ChiJiKisensor.get_angles();

    // 電源基板
    if(led.read() == 0)printf("12V:ON\n");
    else printf("12V:OFF\n");
    printf("電源スイッチ:%d\n\n",!sig.read());
    printf("-------------------------\n\n");
}