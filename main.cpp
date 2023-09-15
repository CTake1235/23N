#include "mbed.h"
#include <cstdio>
#include <string>
#include <map>

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

// PCからのシリアル
UnbufferedSerial pc(A0,A1);

// MDの信号
I2C			motor(D14,D15); //PB_9, PB_8



//電源基板まわり
DigitalOut  sig(PC_12);     //緊急停止（オンオフ）
DigitalIn   led(PC_10);     //状態確認

//エアシリンダーズ
DigitalOut  airF(PA_12); // 前輪
DigitalOut  airB(PB_12); // 後輪

//赤外線センサーズ
AnalogIn    sensorF(PC_4); // まえ
AnalogIn    sensorB(PC_5); // うしろ

void        send(char add, char dat);
void        initMap(void);
void        sensor_reader(void);
void        auto_run(void);

// デバッグ用関数
void        debugger(void);
bool state;

static enum	commands{
	com_NotDefined,
	// 以後要変更
	command1,
	command2,
};
static std::map<std::string, commands> MapofCommands;

static enum options{
	opt_NotDefined,
	// 以後
	options1,
};
static std::map<std::string, options> MapofOptions;


int main(){
    char CommandBuf[64];
	char OptionBuf[64];
	char ValueBuf[64];

	// 電源基板オフ
    sig = 1;

	// 自動足上下オフ
    state = false;

    // 全エアシリを押し下げ
    // 信号が来ないとき、足回りは押し下げられる
    airF.write(0);
    airB.write(0);

	// 自動足上下コマンドの割り込み開始
    runner.attach(&auto_run,30ms);

    while (true) {

        debugger();
        switch(MapofCommands[CommandBuf]){
		case command1:
			switch(MapofOptions[OptionBuf]){
				case options1:
					break;
			}
        // if(select == 1){
        //     sig = 1;
        // }

        // else if(start == 1){
        //     sig = 0;
        // }
        // else if(ue){
        //     send(MIGI_MAE,      FWD);
        //     send(HIDARI_MAE,    FWD);
        //     send(MIGI_USIRO,    FWD);
        //     send(HIDARI_USIRO,  FWD);
        // }
        // else if(sita){
        //     send(MIGI_MAE,      BCK);
        //     send(HIDARI_MAE,    BCK);
        //     send(MIGI_USIRO,    BCK);
        //     send(HIDARI_USIRO,  BCK);
        // }
        // else if(migi){
        //     send(MIGI_MAE,      BCK);
        //     send(HIDARI_MAE,    FWD);
        //     send(MIGI_USIRO,    FWD);
        //     send(HIDARI_USIRO,  BCK);
        // }
        // else if(hidari){
        //     send(MIGI_MAE,      FWD);
        //     send(HIDARI_MAE,    BCK);
        //     send(MIGI_USIRO,    BCK);
        //     send(HIDARI_USIRO,  FWD);
        // }

        // else if(R1){
        //     send(MIGI_MAE,      BCK + 16);
        //     send(HIDARI_MAE,    FWD - 16);
        //     send(MIGI_USIRO,    BCK + 16);
        //     send(HIDARI_USIRO,  FWD - 16);
        // }

        // else if(L1){
        //     send(MIGI_MAE,      FWD - 16);
        //     send(HIDARI_MAE,    BCK + 16);
        //     send(MIGI_USIRO,    FWD - 16);
        //     send(HIDARI_USIRO,  BCK + 16);
        // }

        // // 前エアシリ下げ
        // else if(sankaku && R2)  airF.write(1);

        // // 中エアシリ下げ
        // // else if(R1 && L2)       air2.write(1);

        // // 後エアシリ下げ
        // else if(sankaku && L2)  airB.write(1);

        // // 前エアシリ上げ
        // else if(R2)             airF.write(0);

        // // 中エアシリ上げ
        // // else if(R1)             air2.write(0);

        // // 後エアシリ上げ
        // else if(L2)             airB.write(0);

        // // 自動角材超え開始
        // else if(maru){
        //     state = true;
        //     send(MIGI_MAE,      FWD);
        //     send(HIDARI_MAE,    FWD);
        //     send(MIGI_USIRO,    FWD);
        //     send(HIDARI_USIRO,  FWD);
        // }
        // else{
        //     send(MIGI_MAE,      BRK);
        //     send(HIDARI_MAE,    BRK);
        //     send(MIGI_USIRO,    BRK);
        //     send(HIDARI_USIRO,  BRK);
        // }
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

void initMap(void){
	MapofCommands["ue"] = command1;
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

        sensor_reader();
        debugger();
        if(dis[0] <= WOOD){
            airF.write(1); // 前あげ
        }
        else if(dis[1] <= WOOD){
            airF.write(0);
            ThisThread::sleep_for(10ms);
            airB.write(1);
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