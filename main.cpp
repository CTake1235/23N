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

// duty比に直す前のパワーの「大きさ」
int power[4] = {};

// 計算したduty比を格納するやつ
int	duty[4] = {};

// MDのアドレス、{右前, 左前, 右後, 左後}
const int MDadd[4] = {0x26, 0x54, 0x56, 0x50};

// 各ゲインをゲインゲインするやつ
const float kp = 0.1;
const float ki = 0.001;
const float kd = 0.0;


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
PID		pid_mm(kp, ki, kd, 0.050);
PID		pid_hm(kp, ki, kd, 0.050);
PID 	pid_mu(kp, ki, kd, 0.050);
PID		pid_hu(kp, ki, kd, 0.050);

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
            state = true;
			PIDsetter('u');
        }
        else{
			for(int i = 0; i < 4; i++){
				duty[i] = 0x80;
			}
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
	if(btn == 'R'){

	}
	else if(btn == 'L'){

	}
	else{
		// 入力範囲、目標値の指定 右を向いてるとき
		if(ChiJiKisensor.euler.yaw < 90){
			pid_mm.setInputLimits(0, 90);
			pid_hm.setInputLimits(0, 90);
			pid_mu.setInputLimits(0, 90);
			pid_hu.setInputLimits(0, 90);

			pid_mm.setSetPoint(0);
			pid_hm.setSetPoint(0);
			pid_mu.setSetPoint(0);
			pid_hu.setSetPoint(0);
			
			// 右に傾いているので、右輪は正転方向、左輪は逆転方向に調整する
			pid_mm.setOutputLimits(128,		255);
			pid_hm.setOutputLimits(-128, 	0);
			pid_mu.setOutputLimits(128, 	255);
			pid_hu.setOutputLimits(-128,	0);
		}
		
		// 入力範囲、目標値の指定 左を向いてるとき
		else if(ChiJiKisensor.euler.yaw > 270){
			pid_mm.setInputLimits(270, 360);
			pid_hm.setInputLimits(270, 360);
			pid_mu.setInputLimits(270, 360);
			pid_hu.setInputLimits(270, 360);

			pid_mm.setSetPoint(360);
			pid_hm.setSetPoint(360);
			pid_mu.setSetPoint(360);
			pid_hu.setSetPoint(360);

			// 左に傾いているので、右輪は逆転、左輪は正転
			pid_mm.setOutputLimits(-128, 	0);
			pid_hm.setOutputLimits(128,		255);
			pid_mu.setOutputLimits(-128, 	0);
			pid_hu.setOutputLimits(128, 	255);
		}

		// 入力値をとる
		pid_mm.setProcessValue(ChiJiKisensor.euler.yaw);
		pid_hm.setProcessValue(ChiJiKisensor.euler.yaw);
		pid_mu.setProcessValue(ChiJiKisensor.euler.yaw);
		pid_hu.setProcessValue(ChiJiKisensor.euler.yaw);

		// 出すべき力の大きさを計算
		power[0] = pid_mm.compute();
		power[1] = pid_hm.compute();
		power[2] = pid_mu.compute();
		power[3] = pid_hu.compute();

		// 符号がマイナスならプラスにして逆転の0~128にする
		for(int j = 1; j < 4; j++){
			if(power[j] < 0){
				power[j] = power[j] * -1;
			}
		}

		// ボタンごとに回転方向を変更
		switch (btn){
			case 'u':
				for(int i = 0; i < 4; i++) duty[i] = power[i];
				break;
			case 's':
				for(int i = 0; i < 4; i++) duty[i] = 0xff - power[i];
				break;
			case 'm':
				for(int i = 0; i < 4; i++){
					if(i == 0 || i == 3){
						duty[i] = 0xff - power[i];
					}else {
						duty[i] = power[i];
					}
				}
				break;
			case 'h':
				for(int i = 0; i < 4; i++){
					if(i == 0 || i == 3){
						duty[i] = power[i];
					}else{
						duty[i] = 0xff - power[i];
					}
				}
				break;
		}
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