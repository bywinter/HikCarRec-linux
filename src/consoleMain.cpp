
#ifndef __APPLE__

#include <stdio.h>
#include <iostream>
#include <fstream> 
#include <streambuf> 
#include "GetStream.h"
#include "public.h"
#include "ConfigParams.h"
#include "Alarm.h"
#include "CapPicture.h"
#include "playback.h"
#include "Voice.h"
#include "tool.h"
#include <unistd.h>
#include <time.h>
#include <string.h>
using namespace std;


//参数声明
int iNum = 0; 	//图片名称序号
LONG IUserID;	//摄像机设备
LONG IHandle = -1;//报警布防/撤防;
NET_DVR_DEVICEINFO_V30 struDeviceInfo;	//设备信息

string carNum;//车牌号							
string LineByLine;//逐行读取文件 
char sDVRIP[20]="192.168.2.103";	//抓拍摄像机设备IP地址
short wDVRPort = 8000;	//设备端口号
char sUserName[20]="admin";	//登录的用户名
char sPassword[20]="youPassword";	//用户密码


//函数声明
void Connect();//设置连接事件与重连时间
void CALLBACK MSesGCallback(LONG ICommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, unsigned int dwBufLen, void *pUser);//报警回调函数
void SetMessageCallBack();//设置报警回调函数
void Whitelist();//白名单比对
void Blacklist();//黑名单比对
void SetupAlarm();//报警布防
void CloseAlarm();//报警撤防
void OnExit(void);//退出
				  //---------------------------------------------------------------------------------------------------
				  //函数定义
				  //初始化

//设置连接事件与重连时间
void Connect()
{
	NET_DVR_SetConnectTime(2000, 1);
	NET_DVR_SetReconnect(1000, true);
}



//定义异常消息回调函数
void CALLBACK g_ExceptionCallBack1(unsigned int dwType, LONG lUserID, LONG lHandle, void *pUser)
{
	char tempbuf[256] = { 0 };
	switch (dwType)
	{
	case EXCEPTION_RECONNECT:    //预览时重连  
		printf("----------reconnect--------%d\n", time(NULL));
		break;
	default:
		break;
	}
}


//报警回调函数
void CALLBACK MSesGCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, unsigned int dwBufLen, void* pUser)
{

	ofstream oFile;//定义文件输出流
	oFile.open("carNum.txt", ofstream::app);    //打开要输出的文件 	
											 //获取系统时间
	int i = 0;
	char filename[100];
	FILE *fSnapPic = NULL;
	FILE *fSnapPicPlate = NULL;
	//以下代码仅供参考，实际应用中不建议在该回调函数中直接处理数据保存文件，
	//例如可以使用消息的方式(PostMessage)在消息响应函数里进行处理。
	switch (lCommand) {
		//交通抓拍结果(老报警消息)
	case COMM_UPLOAD_PLATE_RESULT: {
		NET_DVR_PLATE_RESULT struPlateResult = { 0 };
		memcpy(&struPlateResult, pAlarmInfo, sizeof(struPlateResult));
		printf("Car Number: %s\n", struPlateResult.struPlateInfo.sLicense);//车牌号		
		oFile << struPlateResult.struPlateInfo.sLicense << endl;//保存车牌号到txt文件			
																																															//场景图
		if (struPlateResult.dwPicLen != 0 && struPlateResult.byResultType == 1)
		{
			sprintf(filename, "./pic/%s.jpg", struPlateResult.struPlateInfo.sLicense);
			fSnapPic = fopen(filename, "wb");
			fwrite(struPlateResult.pBuffer1, struPlateResult.dwPicLen, 1, fSnapPic);
			iNum++;
			fclose(fSnapPic);
		}
		//车牌图
		if (struPlateResult.dwPicPlateLen != 0 && struPlateResult.byResultType == 1)
		{
			sprintf(filename, "./pic/1/%s.jpg", struPlateResult.struPlateInfo.sLicense);
			fSnapPicPlate = fopen(filename, "wb");
			fwrite(struPlateResult.pBuffer1, struPlateResult.dwPicLen, 1, fSnapPicPlate);
			iNum++;
			fclose(fSnapPicPlate);
		}
		//其他信息处理......
		break;
	}
	//交通抓拍结果(新报警消息)
	case COMM_ITS_PLATE_RESULT: {
		NET_ITS_PLATE_RESULT struITSPlateResult = { 0 };
		memcpy(&struITSPlateResult, pAlarmInfo, sizeof(struITSPlateResult));
		for (i = 0; i < struITSPlateResult.dwPicNum; i++)
		{
			printf("车牌号: %s\n", struITSPlateResult.struPlateInfo.sLicense);//车牌号
			carNum = struITSPlateResult.struPlateInfo.sLicense;
			if ((struITSPlateResult.struPicInfo[i].dwDataLen != 0) && (struITSPlateResult.struPicInfo[i].byType == 1) || (struITSPlateResult.struPicInfo[i].byType == 2))
			{
				//保存图片
				sprintf(filename, "./pic/%s_%d.jpg", struITSPlateResult.struPlateInfo.sLicense, i);
				fSnapPic = fopen(filename, "wb");
				fwrite(struITSPlateResult.struPicInfo[i].pBuffer, struITSPlateResult.struPicInfo[i].dwDataLen, 1, fSnapPic);
				iNum++;
				fclose(fSnapPic);
			}
			//车牌小图片
			if ((struITSPlateResult.struPicInfo[i].dwDataLen != 0) && (struITSPlateResult.struPicInfo[i].byType == 0))
			{
				sprintf(filename, "./pic/1/%s_%d.jpg", struITSPlateResult.struPlateInfo.sLicense, i);
				fSnapPicPlate = fopen(filename, "wb");
				fwrite(struITSPlateResult.struPicInfo[i].pBuffer, struITSPlateResult.struPicInfo[i].dwDataLen, 1, \
					fSnapPicPlate);
				iNum++;
				fclose(fSnapPicPlate);
			}
			//其他信息处理......
		}
		//Whitelist();//白名单比对
		//Blacklist();//黑名单比对
		break;
	}
	default: {
		std::cout << lCommand << endl;
		break;
	}
	}
	oFile.close();//关闭文件
	return;
}

//设置报警回调函数
void SetMessageCallBack()
{
	NET_DVR_SetDVRMessageCallBack_V30(MSesGCallback, NULL);
}


/*
要使 PC 能够收到设备主动发过来的报警等信息，必须将设备的网络配置中的远程报警主机地
址(struAlarmHostIpAddr)设置成 PC 机的 IP 地址（与接口中的 sLocalIP 参数一致），
远程报警主 机端口号(wAlarmHostIpPort)设置成 PC 机的监听端口号（与接口中的 wLocalPort 参数一致）。
*/
//报警布防
void SetupAlarm()
{
	//启动布防
	NET_DVR_SETUPALARM_PARAM struSetupParam = { 0 };
	struSetupParam.dwSize = sizeof(NET_DVR_SETUPALARM_PARAM);
	struSetupParam.byAlarmInfoType = 1;//上传报警信息类型：0-老报警信息(NET_DVR_PLATE_RESULT), 1-新报警信息(NET_ITS_PLATE_RESULT)
	struSetupParam.byLevel = 1;//布防优先级：0- 一等级（高），1- 二等级（中），2- 三等级（低）
							   //bySupport 按位表示，值：0 - 上传，1 - 不上传;  bit0 - 表示二级布防是否上传图片;

	IHandle = NET_DVR_SetupAlarmChan_V41(IUserID, &struSetupParam);//建立报警上传通道，获取报警等信息。
	if (IHandle < 0)
	{
		std::cout << "NET_DVR_SetupAlarmChan_V41 Failed! Error number：" << NET_DVR_GetLastError() << std::endl;
		NET_DVR_Logout(IUserID);
		NET_DVR_Cleanup();
		return;
	}
	std::cout << "报警布防成功！" << endl;
}
//报警撤防
void CloseAlarm()
{
	//撤销布防上传通道
	if (!NET_DVR_CloseAlarmChan_V30(IHandle))//布防句柄IHandle
	{
		std::cout << "NET_DVR_CloseAlarmChan_V30 Failed! Error number：" << NET_DVR_GetLastError() << std::endl;
		NET_DVR_Logout(IUserID);
		NET_DVR_Cleanup();
		return;
	}
	IHandle = -1;//布防句柄;
}
//退出
void OnExit(void)
{
	std::cout << "Begin exit..." << std::endl;
	//报警撤防
	CloseAlarm();
	//释放相机
	    //logout
    NET_DVR_Logout_V30(IUserID);//注销用户
	NET_DVR_Cleanup();//释放SDK资源	
}
//白名单比对
void Whitelist() {

	ifstream iFile;
	iFile.open("白名单.csv", ios::in);
	if (!iFile.is_open())
		std::cout << "找不到文件";
	while (getline(iFile, LineByLine))
	{
		if (LineByLine.empty()) {
			continue;
		}
		else {
			size_t found = LineByLine.find(carNum.substr(4, 8));//carNum.substr(4, 8) 截取车牌号“蓝新NF8202”为NF8202
			if (found != std::string::npos) {
				std::cout << "白名单:" << LineByLine << '\n';
			}

		}
	}
	iFile.close();//关闭文件
}

//黑名单比对
void Blacklist() {

	ifstream iFile;
	iFile.open("黑名单.csv", ios::in);
	if (!iFile.is_open())
		std::cout << "Not Found The File black.csv";

	while (getline(iFile, LineByLine))
	{
		if (LineByLine.empty()) {
			continue;
		}
		else {
			size_t found = LineByLine.find(carNum.substr(4, 8));//carNum.substr(4, 8) 截取车牌号“蓝新NF8202”为NF8202
			if (found != std::string::npos) {
				std::cout << "黑名单:" << LineByLine << '\n';
			}

		}
	}
	iFile.close();//关闭文件
}

int main()
{	
	NET_DVR_Init();
    Demo_SDK_Version();
    NET_DVR_SetLogToFile(3, "./sdkLog");
    char cUserChoose = 'r';
    
	//Login();
	
    //Login device
    NET_DVR_USER_LOGIN_INFO struLoginInfo = {0};
    NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = {0};
    struLoginInfo.bUseAsynLogin = false;

    struLoginInfo.wPort = 8000;
    memcpy(struLoginInfo.sDeviceAddress, "192.168.2.103", NET_DVR_DEV_ADDRESS_MAX_LEN);
    memcpy(struLoginInfo.sUserName, "admin", NAME_LEN);
    memcpy(struLoginInfo.sPassword, "wentuo2020", NAME_LEN);

    IUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);

	if (IUserID < 0)
    {
        printf("NET_DVR_SetupAlarmChan_V41 failed, error code: %d\n", NET_DVR_GetLastError());
		printf("Press any key to quit...\n");
        cin>>cUserChoose;
        NET_DVR_Logout(IUserID);
        NET_DVR_Cleanup(); 
        return HPR_ERROR;
    }
    printf("布防成功!\n");

	Connect();//设置连接事件与重连时间			  	
	SetupAlarm();//布防，此处应该是一级布防
	SetMessageCallBack();	//注册报警回调函数 
	while(1) {
		SetMessageCallBack();	//报警回调函数 	
		sleep(500);		
	}
	OnExit(); 
    return 0;
}
#endif
