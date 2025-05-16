/*
 * bsp_key_app.c
 *
 *  Created on: 2025年2月19日
 *      Author: Administrator
 */

#include "bsp.h"


KEY_T_TYPEDEF key_t;

//void key_add_fun(void);

//void key_dec_fun(void);

uint8_t  set_temp_flag;


typedef struct {
    uint8_t *flag;
    uint8_t threshold;
    void (*onPress)(void);
} KeyHandler;

/*********************************************************************************
 * 
 * Function Name:void ai_on_off_handler(void)
 * 
 * 
 **********************************************************************************/
void SetDataTemperatureValue(void)
{
    if(set_temp_flag ==1){
	 set_temp_flag++;

     //SendData_Tx_Data(0x11,gpro_t.set_up_temperature_value);
     SendData_ToMainboard_Data(0x2A,&gpro_t.set_up_temperature_value,0x01);
     osDelay(5);
	}  


}

/*********************************************************************************
 * 
 * Function Name:void mouse_on_off_handler(void)
 * // 设置温度并做边界检查
 * 
 **********************************************************************************/
void set_temperature_value(int8_t delta) 
{
    uint8_t new_temp;
	static uint8_t temperature_init_value;

	if(temperature_init_value == 0 && gpro_t.set_temp_value_success==0){
        temperature_init_value++;
        gpro_t.set_up_temperature_value = (delta > 0) ? 21 : 39;
	    new_temp = gpro_t.set_up_temperature_value;
    }
	else{

	   	new_temp = gpro_t.set_up_temperature_value + delta;
	    if (new_temp < 20) new_temp = 20;
        if (new_temp > 40) new_temp = 40;
   }

	

   

    gpro_t.set_up_temperature_value = new_temp;

    run_t.set_temperature_decade_value = new_temp / 10;
    run_t.set_temperature_unit_value   = new_temp % 10;

    run_t.set_temperature_special_flag = 1;
    run_t.gTimer_key_temp_timing       = 0;
    gpro_t.g_manual_shutoff_dry_flag   = 0;
    set_temp_flag                      = 1;

    SendData_ToMainboard_Data(0x2A,&new_temp,0x01);
    osDelay(5);

    TM1639_Write_2bit_SetUp_TempData(run_t.set_temperature_decade_value, run_t.set_temperature_unit_value, 0);
	
}

/*******************************************************
	*
	*Function Name: void bsp_plasma_handler(uint8_t data)
	*Function :
	*
	*
*******************************************************/
void adjust_timer_minutes(int8_t delta_min) 
{
    int8_t total_hour = run_t.temporary_timer_dispTime_hours ;
	uint8_t copy_total_hour;
    total_hour += delta_min;

   if(total_hour > 24){
         total_hour =0;
   	}
	else if (total_hour < 0) {
        total_hour = 24 ;  // 循环处理负值
    }

   // total_hour %= 24 ;  // 保证在一天范围内

    run_t.temporary_timer_dispTime_hours   = total_hour;
    run_t.temporary_timer_dispTime_minutes = 0;

    run_t.hours_two_decade_bit    = run_t.temporary_timer_dispTime_hours / 10;
    run_t.hours_two_unit_bit      = run_t.temporary_timer_dispTime_hours % 10;
    run_t.minutes_one_decade_bit  = 0;
    run_t.minutes_one_unit_bit    = 0;
	gpro_t.input_numbers_flag++;

	copy_total_hour=(uint8_t)total_hour;
	SendData_ToMainboard_Data(0x4C,&copy_total_hour,0x01);
	osDelay(5);

    // TM1639_Write_4Bit_Time(run_t.hours_two_decade_bit, run_t.hours_two_unit_bit,
    //                        run_t.minutes_one_decade_bit, run_t.minutes_one_unit_bit, 0);
}

/****************************************************************
	*
	*Function Name :void handle_key(KeyHandler *handler) 
	*Function : set timer timing how many ?
	*Input Parameters : struct KeyHandler of reference
	*Retrurn Parameter :NO
	*
*****************************************************************/
void handle_key(KeyHandler *handler) 
{
    if (*(handler->flag) == 1) {
        *(handler->flag) += 1;

        if (handler->threshold > 0 && *(handler->flag) > handler->threshold) {
            *(handler->flag) = 80; // 特殊情况处理
        }

        if (handler->onPress) {
            handler->onPress();
        }
    }
}




/**********************************************************************************************************
*	函 数 名: bsp_PutKey
*	功能说明: 将1个键值压入按键FIFO缓冲区。可用于模拟一个按键。
*	形    参:  _KeyCode : 按键代码
*	返 回 值: 无
**********************************************************************************************************/
void sendCommandAndAck(uint8_t cmd, uint8_t value, uint8_t ackType) {
    SendData_Set_Command(cmd, value);
    osDelay(5);
    gpro_t.send_ack_cmd = ackType;
    gpro_t.gTimer_again_send_power_on_off = 0;
}

/**********************************************************************************************************
*	函 数 名: void power_key_handler(void) 
*	功能说明: 从按键FIFO缓冲区读取一个键值。
*	形    参:  无
*	返 回 值: 按键代码
**********************************************************************************************************/
void power_key_handler(void) 
{
    if(run_t.gPower_On == power_off){
        SendData_PowerOnOff(1); // power on
    } else {
        SendData_PowerOnOff(0); // power off
    }
    osDelay(10);
}


/**********************************************************************************************************
*	函 数 名: void plasma_key_handler(void) 
*	功能说明: 从按键FIFO缓冲区读取一个键值。
*	形    参:  无
*	返 回 值: 按键代码
**********************************************************************************************************/
void plasma_key_handler(void) 
{
    if(gpro_t.set_timer_timing_doing_value==0 ||gpro_t.set_timer_timing_doing_value==3){
        if(run_t.gPlasma == 1){
            run_t.gPlasma = 0;
            SendData_Set_Command(plasma_cmd, 0x00);
            LED_PLASMA_OFF();
            gpro_t.send_ack_cmd = check_ack_plasma_off;
        } else {
            run_t.gPlasma = 1;
            SendData_Set_Command(plasma_cmd, 0x01);
            LED_PLASMA_ON();
            gpro_t.send_ack_cmd = check_ack_plasma_on;
        }
        gpro_t.gTimer_again_send_power_on_off = 0;
    }
}
/****************************************************************
	*
	*Function Name :void mode_key_handler(void)
	*Function : set timer timing how many ?
	*Input Parameters :NO
	*Retrurn Parameter :NO
	*
*****************************************************************/

void dry_key_handler(void) 
{
    if(gpro_t.set_timer_timing_doing_value == 0 || gpro_t.set_timer_timing_doing_value == 3) {
        if(run_t.gDry == 0) {
            sendCommandAndAck(dry_cmd, 0x01, check_ack_ptc_on);
            run_t.gDry = 1;
            gpro_t.g_manual_shutoff_dry_flag = 0;
            LED_DRY_ON();
        } else {
            sendCommandAndAck(dry_cmd, 0x00, check_ack_ptc_off);
            run_t.gDry = 0;
            gpro_t.g_manual_shutoff_dry_flag = 1; // 手动关闭后不再自动开启
            LED_DRY_OFF();
        }
    }
}
/****************************************************************
	*
	*Function Name :void mode_key_handler(void)
	*Function : set timer timing how many ?
	*Input Parameters :NO
	*Retrurn Parameter :NO
	*
*****************************************************************/

void mouse_key_handler(void) 
{
    if(gpro_t.set_timer_timing_doing_value == 0 || gpro_t.set_timer_timing_doing_value == 3) {
        if(run_t.gMouse == 0) {
            // 开启 Mouse 功能
            SendData_Set_Command(mouse_cmd, 0x01);
            osDelay(5);
            run_t.gMouse = 1;
            LED_MOUSE_ON();
            gpro_t.send_ack_cmd = check_ack_mouse_on;  // 假设有对应的反馈类型
            gpro_t.gTimer_again_send_power_on_off = 0;

        } else if(run_t.gMouse == 1) {
            // 关闭 Mouse 功能
            SendData_Set_Command(mouse_cmd, 0x00);
            osDelay(5);
            run_t.gMouse = 0;
            LED_MOUSE_OFF();
            gpro_t.send_ack_cmd = check_ack_mouse_off;  // 假设有对应的反馈类型
            gpro_t.gTimer_again_send_power_on_off = 0;
        }
    }
}
/****************************************************************
	*
	*Function Name :void mode_key_handler(void)
	*Function : set timer timing how many ?
	*Input Parameters :NO
	*Retrurn Parameter :NO
	*
*****************************************************************/
void key_add_fun(void)
{
    if(run_t.ptc_warning != 0) return;

    run_t.gTimer_time_colon = 0;

    switch(gpro_t.set_timer_timing_doing_value)
    {

	    case 3:
		case 0:  // 设置温度增加
           // SendData_Buzzer();
		   // osDelay(5);
            set_temperature_value(+1);
            break;

        case 1:  // 设置定时增加（每次加60分钟）
            SendData_Buzzer();
			 osDelay(5);
            run_t.gTimer_key_timing = 0;

            adjust_timer_minutes(1);  // 固定每次加60分钟
            break;
    }
}


void key_dec_fun(void)
{
    if(run_t.ptc_warning != 0) return;

    switch(gpro_t.set_timer_timing_doing_value)
    {

        case 3:
		case 0:  // 设置温度减少
            //SendData_Buzzer();
		    // osDelay(5);
            set_temperature_value(-1);
            break;

        case 1:  // 设置定时减少（每次减60分钟）
            SendData_Buzzer();
			 osDelay(5);
            run_t.gTimer_key_timing = 0;

            adjust_timer_minutes(-1);  // 固定每次减60分钟
        break;
    }
}
/****************************************************************
	*
	*Function Name :void mode_key_handler(void)
	*Function : set timer timing how many ?
	*Input Parameters :NO
	*Retrurn Parameter :NO
	*
*****************************************************************/
uint8_t mode_key_handler(void)
{

   
	if(MODEL_KEY_VALUE() == KEY_DOWN && gpro_t.mode_Key_long_counter < 200){
		gpro_t.mode_Key_long_counter++;

	    if(gpro_t.mode_Key_long_counter > 100 ){
           
              
		
		  gpro_t.mode_Key_long_counter=220;
		   if(gpro_t.DMA_txComplete ==1){

		   gpro_t.DMA_txComplete=0;
		   SendData_Buzzer();
		 
		   }
		  // mode_key_long_fun();
		   return 0x81;


		}



	}
	
    if(MODEL_KEY_VALUE() == KEY_UP  &&  key_t.key_mode_flag==1 && gpro_t.mode_Key_long_counter != 220){

           key_t.key_mode_flag++;
		   gpro_t.mode_Key_long_counter=0;

		   SendData_Buzzer();
		   osDelay(5);
		  // mode_key_short_fun();

           return 0x05;
        	

	}
	else if(MODEL_KEY_VALUE() == KEY_UP  && gpro_t.mode_Key_long_counter ==220){
	      gpro_t.mode_Key_long_counter=0;

	}

	return 0;

}

void mode_key_parse(uint8_t keyvalue)
{


    // 1. 系统状态检查
    if (run_t.gPower_On != power_on) {
        //gpro_t.mode_Key_long_counter = 0;
        return;
    }

   switch(keyvalue){

    case 0x81:
   	
        
	     gpro_t.mode_Key_long_counter=0;
		
   	  
        mode_key_long_fun();
    break;

	case 0x01:
	     gpro_t.mode_Key_long_counter=0;
		 SendData_Buzzer();
		 osDelay(5);
		 gpro_t.mode_key_shot_flag = 1;
         //mode_key_short_fun();
	   	

	 break;

   	}
  
}

#if 0
// 按键参数配置（可全局调整）
#define KEY_LONG_PRESS_THRESHOLD   30      // 300ms长按阈值
#define DEBOUNCE_TIME_MS          5       // 消抖时间
#define KEY_SCAN_INTERVAL_MS      10      // 按键扫描间隔

// 优化后的按键处理函数
void mode_key_handler(void)
{
    static uint8_t debounce_counter = 0;
    static uint8_t stable_state = KEY_UP;
    static bool long_press_flag = false;
    
    // 1. 系统状态检查
//    if (run_t.gPower_On != power_on) {
//        gpro_t.mode_Key_long_counter = 0;
//        return;
//    }

    // 2. 硬件消抖处理（更稳定的状态检测）
    uint8_t current_state = MODEL_KEY_VALUE();

	
	
    // 3. 按键状态处理（精简逻辑）
    if (current_state == KEY_DOWN && key_t.key_mode_flag==1) {
        // 按下瞬间立即触发短按
            key_t.key_mode_flag++;
		    gpro_t.mode_key_shot_flag = 1;
            SendData_Buzzer();
			osDelay(5);
           // mode_key_short_fun();
            gpro_t.mode_Key_long_counter=0;
    }
    else if (current_state == KEY_DOWN && gpro_t.mode_Key_long_counter< 60) {// 长按计数及触发
            gpro_t.mode_Key_long_counter++;
            
            if (gpro_t.mode_Key_long_counter >= KEY_LONG_PRESS_THRESHOLD) {
				key_t.key_mode_flag++;
				gpro_t.mode_Key_long_counter=80;
                SendData_Buzzer();
				osDelay(5);
                mode_key_long_fun();
                long_press_flag = true;
            }
     } 
    else {
        // 释放时重置状态
        if (gpro_t.mode_Key_long_counter > 0) {
            gpro_t.mode_Key_long_counter = 0;
            long_press_flag = false;
        }
    }
}

#endif 
/*
*********************************************************************************************************
*	函 数 名: void process_keys(void) 
*	功能说明:
*	形    参：无
*	返 回 值: 按键代码
*********************************************************************************************************
*/
void process_keys(void) 
{
    // 处理WiFi键
    if(WIFI_KEY_VALUE() == KEY_DOWN && key_t.key_wifi_flag < 60 && run_t.gPower_On == power_on) {
        key_t.key_wifi_flag++;
        if(key_t.key_wifi_flag > 30) {
            key_t.key_wifi_flag = 80;
            SendData_Buzzer();
			osDelay(5);
        }
    }
//    else{
//	   mode_key_handler() ;
//    }

    // 定义所有按键处理器
    KeyHandler handlers[] = {
        { &key_t.key_power_flag, 0, power_key_handler },
        //{ &key_t.key_mode_flag, 0, mode_key_handler },
        { &key_t.key_dec_flag, 0, key_dec_fun },
        { &key_t.key_add_flag, 0, key_add_fun },
        { &key_t.key_plasma_flag, 0, plasma_key_handler },
        { &key_t.key_dry_flag, 0, dry_key_handler },
        { &key_t.key_mouse_flag, 0, mouse_key_handler },
    };

    // 循环处理每个按键
    for (int i = 0; i < sizeof(handlers)/sizeof(handlers[0]); i++) {
        handle_key(&handlers[i]);
    }
}



