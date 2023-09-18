#include "mbed.h"
#include "PS3.h"
#include "BNO055.h"
#include <cstdio>
#include "PIDcontroller.h"

#define WOOD            90 // [mm]

Ticker	stopper;

// 計算前、デフォルトのduty比
const int  rawFWD = 0xa8;
const int  rawBCK = 0xff - rawFWD;
const int  BRK = 0x80;

// MDのアドレス、{右前, 左前, 右後, 左後}
const int MDadd[4] = {0x26, 0x54, 0x56, 0x50};

int	duty[4] = {};


PS3     ps3(D8,D2);     //PA_9,PA_10
I2C     motor(D14,D15); //PB_9, PB_8
BNO055	ChiJiKisensor(PB_4,PA_8);
/*
#hanagehogehoge
地磁気センサーの値のみかた[deg]
^: ロボット、頂点が前
センサーをてっぺんから見た図

	0
270 ^ 90
	180

*/

// PID(比例ゲイン、積分ゲイン、微分ゲイン、制御周期)
PID		pid_mm(5.2, 0.0, 2.5, 0.050);
PID		pid_hm(5.2, 0.0, 2.5, 0.050);
PID 	pid_mu(5.2, 0.0, 2.5, 0.050);
PID		pid_hu(5.2, 0.0, 2.5, 0.050);

//電源基板まわり
DigitalOut  sig(PC_12);     //緊急停止（オンオフ）
DigitalIn   led(PC_10);     //状態確認

//エアシリンダーズ
DigitalOut  air1(PA_12); // 前輪
DigitalOut  air3(PB_12); // 後輪

DigitalOut  myled(LED1);

//赤外線センサーズ
AnalogIn    RF(PA_6); // 右前
AnalogIn    LF(PA_7); // 左前
AnalogIn    RC(PC_5); // 右中
AnalogIn    LC(PC_4); // 左中、これメイン
AnalogIn    RB(PC_2); // 右後
AnalogIn    LB(PC_3); // 左後

void	send(int,int);
void    getdata(void);
void    sensor_reader(void);
void    auto_run(void);
void    stater(void);
void	PIDsetter(char);

// デバッグ用関数
void	debugger(void);
bool	ue,sita,migi,hidari,select,start,batu,maru,sankaku,R1,R2,L1,L2;
bool	state;

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

    // 0:右前 1:左前 2:右後 3:左後
    char motor_duty[4] = {rawFWD,rawFWD,rawFWD,rawFWD};

    // 地磁気センサー初期化、見つかるまでLチカ
    ChiJiKisensor.reset();
    while(!ChiJiKisensor.check())myled.write(!myled.read());
    myled.write(1); // 見つかったら光らっせぱにしておく

	ps3.myattach();
	stopper.attach(&stater, 100ms);

    while (true) {
        sensor_reader();
        auto_run();
        debugger();

        if(select == 1){
            sig = 1;
        }
        else if(start == 1){
            sig = 0;
        }
        else if(ue){
			PIDsetter('u');
        }
        else if(sita){
			PIDsetter('s');
        }
        else if(migi){
			PIDsetter('m');
        }
        else if(hidari){
			PIDsetter('h');
        }

        else if(R1){
			PIDsetter('R');
        }

        else if(L1){
			PIDsetter('L');
        }


        // 自動角材超え開始
        else if(maru){
            PIDsetter('a');
        }
        else{
			PIDsetter('b');
        }
		
		for(int i = 0; i < 4; i++){
			send(MDadd[i], duty[i]);
		}

		// ここからエアシリ手動制御

        // 前エアシリ下げ
        if(sankaku && R2)  air1.write(1);

        // 後エアシリ下げ
        else if(sankaku && L2)  air3.write(1);

        // 前エアシリ上げ
        else if(R2)             air1.write(0);

        // 後エアシリ上げ
        else if(L2)             air3.write(0);

    }
}

void send(int add, int dat){
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
				stopper.detach();
                state = false;
        }
    }
}

void stater(void){
    if(batu){
        state = false;
    }
    else{
        printf("running!\n");
    }
}

void PIDsetter(char btn){

	// #hanagehogehogeを思い出せ


	// セクター1: 入力範囲の指定と目標値の指定

	// 右を向いてるとき
	if(ChiJiKisensor.euler.yaw < 90){
		pid_mm.setInputLimits(0, 90);
		pid_hm.setInputLimits(0, 90);
		pid_mu.setInputLimits(0, 90);
		pid_hu.setInputLimits(0, 90);

		pid_mm.setSetPoint(0);
		pid_hm.setSetPoint(0);
		pid_mu.setSetPoint(0);
		pid_hu.setSetPoint(0);
	}
	
	// 左を向いてるとき
	else if(ChiJiKisensor.euler.yaw > 270){
		pid_mm.setInputLimits(270, 360);
		pid_hm.setInputLimits(270, 360);
		pid_mu.setInputLimits(270, 360);
		pid_hu.setInputLimits(270, 360);

		pid_mm.setSetPoint(360);
		pid_hm.setSetPoint(360);
		pid_mu.setSetPoint(360);
		pid_hu.setSetPoint(360);
	}
	// セクター1ここまで

	// セクター2: 出力範囲の設定
	switch (btn){
		case 'u':
			// 出力範囲の設定
			pid_mm.setOutputLimits(0x80, rawFWD);
			pid_hm.setOutputLimits(0x80, rawFWD);
			pid_mu.setOutputLimits(0x80, rawFWD);
			pid_hu.setOutputLimits(0x80, rawFWD);

			break;
	}

}

void debugger(void){
    // 赤外線センサーのデータ
    // printf("value:\n右前: %f\t左前: %f\n右中: %f\t左中: %f\n右後: %f\t左後: %f\n",value[0],value[1],value[2],value[3],value[4],value[5]);
    printf("value: %lf\n",dis);

	// 地磁気センサーのyaw値
	printf("%f",ChiJiKisensor.euler.yaw);

    // 電源基板
    if(led.read() == 0)printf("12V:ON\n");
    else printf("12V:OFF\n");
    printf("電源スイッチ:%d\n\n",!sig.read());
    printf("-------------------------\n\n");
}