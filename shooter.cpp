#include <iostream>
#include <string>

using namespace std;

int main(){
	string input;
	string command;
	string duration;
	cout << "Shooter läuft" << endl;
	while(true){
		cin >> input
		if (input == "exit"){
			cout << "Beende Shooter" << endl
		} else {
			duration = input.substr(1); //Ausser dem ersten Zeichen sind alle Angaben Zeit angaben
	}
	
	
	return 0;
}
