/* KayboardThermals.cpp : Defines the entry point for the console application.

This program changes the RGB LEDS for Corsair Keyboards based on a Nvidia GPU temperature.
Blue = Cool. Red = hot.
Written by David Keizer based off an NvAPI example on stack overflow, and the examples in the Corsair Cue SDK.

This may require the corsair cue SDK and NvAPI to be installed in the same directory.
I have no idea how this will work on other computers.

Based on the stackoverflow forum post here: -> http://stackoverflow.com/questions/15709958/get-temperature-from-nvidia-gpu-using-nvapi
Personal Note: I am not familiar with the NvAPI so forgive me if my documentation is sparse/incorrect
For more information ->http://docs.nvidia.com/gameworks/content/gameworkslibrary/coresdk/nvapi/index.html
I am also not the most experienced programmer, so feel free to make changes.

I do not have any rights to any of the code found in this program.
This was a personal project for me, just for fun.
*/

#include "CUESDK.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <future>
#include <vector>
#include <windows.h>
#include "nvapi.h"

//Method author: Corsair. Found in the Corsair Cue SDK under the color_pulse.cpp example.
//Method returns an error message
const char* toString(CorsairError error) 
{
	switch (error) {
	case CE_Success : 
		return "CE_Success";
	case CE_ServerNotFound:
		return "CE_ServerNotFound";
	case CE_NoControl:
		return "CE_NoControl";
	case CE_ProtocolHandshakeMissing:
		return "CE_ProtocolHandshakeMissing";
	case CE_IncompatibleProtocol:
		return "CE_IncompatibleProtocol";
	case CE_InvalidArguments:
		return "CE_InvalidArguments";
	default:
		return "unknown error";
	}
}

//Method author: Corsair. Found in the Corsair Cue SDK under the color_pulse.cpp example.
//Methods returns all the available keys on the keyboard, as in the ones that are not lit.
std::vector<CorsairLedColor> getAvailableKeys()
{
	auto colorsVector = std::vector<CorsairLedColor>();
	for (auto deviceIndex = 0; deviceIndex < CorsairGetDeviceCount(); deviceIndex++) {
		if (auto deviceInfo = CorsairGetDeviceInfo(deviceIndex)) {
			switch (deviceInfo->type) {
			case CDT_Mouse: {
				auto numberOfKeys = deviceInfo->physicalLayout - CPL_Zones1 + 1;
				for (auto i = 0; i < numberOfKeys; i++) {
					auto ledId = static_cast<CorsairLedId>(CLM_1 + i);
					colorsVector.push_back(CorsairLedColor{ ledId, 0, 0, 0 });
				}
			} break;
			case CDT_Keyboard: {
				auto ledPositions = CorsairGetLedPositions();
				if (ledPositions) {
					for (auto i = 0; i < ledPositions->numberOfLed; i++) {
						auto ledId = ledPositions->pLedPosition[i].ledId;
						colorsVector.push_back(CorsairLedColor{ ledId, 0, 0, 0 });
					}
				}
			} break;
			case CDT_Headset: {
				colorsVector.push_back(CorsairLedColor{ CLH_LeftLogo, 0, 0, 0 });
				colorsVector.push_back(CorsairLedColor{ CLH_RightLogo, 0, 0, 0 });
			} break;
			}
		}
	}
	return colorsVector;
}

//Method author: Corsair. Found in the Corsair Cue SDK under the color_pulse.cpp example.
double getKeyboardWidth(CorsairLedPositions *ledPositions)
{
	const auto minmaxLeds = std::minmax_element(ledPositions->pLedPosition, ledPositions->pLedPosition + ledPositions->numberOfLed,
		[](const CorsairLedPosition &clp1, const CorsairLedPosition &clp2) {
		return clp1.left < clp2.left;
	});
	return minmaxLeds.second->left + minmaxLeds.second->width - minmaxLeds.first->left;
}

//Method Author: Bigfootmandude
//Based on the stackoverflow forum post here:  http://stackoverflow.com/questions/15709958/get-temperature-from-nvidia-gpu-using-nvapi
//Personal Note: I am not familiar with the NvAPI so forgive me if my documentation is sparse/incorrect
//For more information http://docs.nvidia.com/gameworks/content/gameworkslibrary/coresdk/nvapi/index.html
int getTemp()
{

	//ret is the error handling portion of NvAPI.
	//If NvAPI is not OK then an error has occured and should be handled
	NvAPI_Status ret = NVAPI_OK;
	
	NvDisplayHandle hDisplay_a[NVAPI_MAX_PHYSICAL_GPUS * 2] = { 0 };

	ret = NvAPI_Initialize();
	if (ret != NVAPI_OK) {
		NvAPI_ShortString string;
		NvAPI_GetErrorMessage(ret, string);
		printf("NVAPI NvAPI_Initialize: %s\n", string);
	}

	NvAPI_ShortString ver;
	NvAPI_GetInterfaceVersionString(ver);
	NvU32 cnt = 1; 

	
	//Create the handler for the GPU
	NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS];

	ret = NvAPI_EnumPhysicalGPUs(nvGPUHandle, &cnt);	
	if (ret != NVAPI_OK) {
		NvAPI_ShortString string;
		NvAPI_GetErrorMessage(ret, string);
		printf("NVAPI NvAPI_EnumPhysicalGPUs: %s\n", string);
	}
	
	NvU32 count = 1;

	//Set up the thermal settings. 
	NV_GPU_THERMAL_SETTINGS thermal;
	thermal.version = NV_GPU_THERMAL_SETTINGS_VER_2;
	thermal.count = 1;
	thermal.sensor[0].controller = NVAPI_THERMAL_CONTROLLER_GPU_INTERNAL;
	thermal.sensor[0].target = NVAPI_THERMAL_TARGET_GPU;


	ret = NvAPI_GPU_GetFullName(*nvGPUHandle, name);
	if (ret != NVAPI_OK) {
		NvAPI_ShortString string;
		NvAPI_GetErrorMessage(ret, string);
		printf("NVAPI NvAPI_GPU_GetFullName: %s\n", string);
	}

	

	thermal.version = NV_GPU_THERMAL_SETTINGS_VER;
	ret = NvAPI_GPU_GetThermalSettings(*nvGPUHandle, 0, &thermal);

	if (ret != NVAPI_OK) {
		NvAPI_ShortString string;
		NvAPI_GetErrorMessage(ret, string);
		printf("NVAPI NvAPI_GPU_GetThermalSettings: %s\n", string);
	}
	
	//get the current temperature for the chosen GPU. 
	int temp = thermal.sensor[0].currentTemp;

	//release the resources that NvAPI has allocated
	//No more NvAPI methods can be called after a successful unload.
	ret = NvAPI_Unload();
	if (ret != NVAPI_OK)
	{
		NvAPI_ShortString string;
		NvAPI_GetErrorMessage(ret, string);
		printf("NVAPI NvAPI_EnumPhysicalGPUs: %s\n", string);
	}

	return temp;

	

}

//Method Author: Bigfootmandude
//Calculates the temperature as a percentage based on a temperature range.
//Currently set to handle 25-80 degrees Celcius
//I intend on changing this method in the future, and make it versatile.
double percentFromTemp(int temp)
{
	double perc = temp - 25;
	perc = perc / 60;
	return perc;

}

//Method Author: Bigfootmandude
//Method takes a key LED and updates the color based on the temperature
//This is where the color curve is handled.
//I admite I am not happy with this and will change the numbers to make some nicer colors around the 50 degree mark
CorsairLedColor colorFromTemp(CorsairLedColor color, int temp)
{
	//calculate the color for the LED
	CorsairLedColor newColor;
	int r = (percentFromTemp(temp)) * 256;
	int g = (1 - (percentFromTemp(temp))) * 192;
	int b = (1 - (percentFromTemp(temp))) * 192;

	//updage the LED color
	newColor.ledId = color.ledId;
	newColor.r = r;
	newColor.g = g;
	newColor.b = b;

	return newColor;
}


//Method Author: Bigfootmandude
//Based off of the Corsair Cue SDK under the color_pulse.cpp example.
int main()
{
	//Communicate with the Corsair driver to get access to the keyboard
	CorsairPerformProtocolHandshake();

	//error check
	if (const auto error = CorsairGetLastError()) {
		std::cout << "Handshake failed: " << toString(error) << std::endl;
		getchar();
		return -1;
	}

	
	auto colorsVector = getAvailableKeys();
	if (!colorsVector.empty()) {

		CorsairLedColor key;
		CorsairLedColor arrayColors[1] = {};
		int temp = getTemp();

		//main loop
		//VK_F12 is the key ID for F12. This is used to exit the program. 
		while (!GetAsyncKeyState(VK_F12)) {
			
			temp = getTemp();

			//This is essentially a for each key loop
			for (int i = 0; i < colorsVector.size(); i++)
			{
				key = colorFromTemp(colorsVector.at(i), temp);
				arrayColors[0] = key;
				
				//Update the keyboard
				CorsairSetLedsColors(1, arrayColors);

			}
			//This is the update frequency. I find at once a second, it allows for a smooth color transition with no flickering
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}
	return 0;
}

