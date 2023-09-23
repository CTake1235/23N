#include "mbed.h"
#include "PS3.h"
// #include "HMC5883L.h"
#include <cstdio>

// MDのアドレス
#define MIGI_MAE        0x26
#define HIDARI_MAE      0x54
#define MIGI_USIRO      0x56
#define HIDARI_USIRO    0x50

#define WOOD            100 // [mm]

// 車輪の前進、後退、ブレーキ、ゆっくり（角材超え）
// const char  FWD = 0xe0;
// const char  BCK = 0x20;
const char  FWD = 0x98 + 16;
const char  BCK = 0x62 - 16;
const char  BRK = 0x80;

double      dis[2] = {};
float       value[2];

Ticker      getter;
Ticker      runner;

PS3         ps3		(PA_0, PA_1);
I2C         motor	(PB_9, PC_6);

//電源基板まわり
DigitalOut  sig(PA_11);     //緊急停止（オンオフ）
DigitalIn   led(PA_12);     //状態確認

//エアシリンダーズ
DigitalOut  airF(PC_15); // 前輪
DigitalOut  airB(PH_1); // 後輪

//赤外線センサーズ
AnalogIn    sensorB(PA_6); // 右中
AnalogIn    sensorF(PA_7); // 左中、これメイン

void        send(char add, char dat);
void        getdata(void);
void        sensor_reader(void);
// void        autorun(void);
void        auto_run(void);
void        stater(void);

// デバッグ用関数
void        debugger(void);
bool ue,sita,migi,hidari,select,start,batu,maru,sankaku,R1,R2,L1,L2;
bool state;

int main(){
    sig = 1;
    state = false;
    // 全エアシリをオンにする
    // 信号が来ないとき、足回りは展開されている
    airF.write(0);
    airB.write(0);

    getter.attach(&getdata,1ms); // こっちは1msごとに必ず割り込む
    // ps3.myattach(); // こっちだと受信したときに割り込む

    runner.attach(&auto_run,30ms);

    while (true) {

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
            send(MIGI_MAE,      BCK + 16);
            send(HIDARI_MAE,    FWD - 16);
            send(MIGI_USIRO,    BCK + 16);
            send(HIDARI_USIRO,  FWD - 16);
        }

        else if(L1){
            send(MIGI_MAE,      FWD - 16);
            send(HIDARI_MAE,    BCK + 16);
            send(MIGI_USIRO,    FWD - 16);
            send(HIDARI_USIRO,  BCK + 16);
        }

        // 前エアシリ下げ
        else if(sankaku && R2)  airF.write(1);

        // 中エアシリ下げ
        // else if(R1 && L2)       air2.write(1);

        // 後エアシリ下げ
        else if(sankaku && L2)  airB.write(1);

        // 前エアシリ上げ
        else if(R2)             airF.write(0);

        // 中エアシリ上げ
        // else if(R1)             air2.write(0);

        // 後エアシリ上げ
        else if(L2)             airB.write(0);

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
    L1      = ps3.getButtonState(PS3::L1);
    L2      = ps3.getButtonState(PS3::L2);

    sankaku = ps3.getButtonState(PS3::sankaku);
    maru    = ps3.getButtonState(PS3::maru);
    batu    = ps3.getButtonState(PS3::batu);

}

void sensor_reader(void){
    value[0] = sensorF.read();
    value[1] = sensorB.read();
    for(int i = 0; i < 2; i++){
        dis[i] = 71.463 * pow(value[i],-1.084);
    }
}

void auto_run(void){
    while(state){
        int flag = 0;
        bool finish = false;
        // ここでもgetdataがちゃんとattachしてるかチェック
        printf("%d\n",batu);

        sensor_reader();
        debugger();
        if(dis[0] <= WOOD && flag == 0){
            airF.write(1); // 前あげ
        }
        else if(dis[1] <= WOOD && flag == 1){
            airF.write(0);
            ThisThread::sleep_for(100ms);
            airB.write(1);
            for(int i = 0; i < 100; i++){
                if(!state) {
                    finish = true;
                    break;
                }
                ThisThread::sleep_for(10ms);
            }
            airB.write(0);
            if(finish){
                finish = false;
                flag = 0;
                break;
            }
        }
            // for(int counter = 0;counter < 4;counter++){
            //     if(!state)break;
            //     switch (counter) {
            //         case 0:
            //             airF = 1;
            //             printf("前あげ\n");
            //             ThisThread::sleep_for(1s);
            //             break;
            //         case 1:
            //             airF = 0;
            //             printf("前さげ\n");
            //             ThisThread::sleep_for(100ms);
            //             break;
            //         case 2:
            //             airB = 1;
            //             printf("後あげ\n");
            //             ThisThread::sleep_for(1s);
            //             break;
            //         case 3:
            //             airB = 0;
            //             printf("後さげ\n");
            //             break;
            //     }
        state = false;
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
    printf("sensorF: %lf\nsensorB:%lf\n",dis[0],dis[1]);

    // 地磁気センサーの値（見るだけ）
    // printf("角度:\t%f\n",ChiJiKisensor.getHeadingXYDeg());

    // 電源基板
    if(led.read() == 0)printf("12V:ON\n");
    else printf("12V:OFF\n");
    printf("電源スイッチ:%d\n\n",!sig.read());
    printf("-------------------------\n\n");
}