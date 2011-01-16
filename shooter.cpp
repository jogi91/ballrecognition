#include <iostream>
#include <string>
#include <stdlib.h>

using namespace std;

int main(){
	string input;
	string command;
	string flips;
	string durationstring;
	int duration;
	cout << "Shooter läuft" << endl;
	while(true){
		cin >> input;
		cout << input << endl;
		if (input == "exit"){
			cout << "Beende Shooter" << endl;
			break;
		} else {
			flips = input[0];
			command = "echo -n "+flips+" >> /dev/ttyUSB0";
			system(command.c_str());
			durationstring = input.substr(1); //Ausser dem ersten Zeichen sind alle Angaben Zeit angaben
			duration = atoi(durationstring.c_str());
			if (duration != 0) { // Eine Zeitangabe von 0 bedeutet so lange, bis ein anderer Befehl kommt
				usleep(duration*1000);
				system("echo -n 0 >> /dev/ttyUSB0");
			}
		}
	}
	return 0;
}
