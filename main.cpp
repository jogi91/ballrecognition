#include <stdio.h>
#include "cv.h"
#include "highgui.h"
#include <iostream>
#include <fstream>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
using namespace std;

#define LINKS  2
#define RECHTS 1
#define BEIDE 3
//Globale Variablen fuer die Referenzbilder, momentan wird nur refFlipsDown verwendet
IplImage *refFlipsDown = 0;
IplImage *refRightFlipUp = 0;
IplImage *refLeftFlipUp = 0;
IplImage *refBothFlipsUp =0;

//Schiesst, wartet delay millisekunden, und l�sst dann wieder los, und das alles mit den gew�nschten Flipperfingern
void shoot(int delay, int flips){
	cout << flips << delay << endl; //cout wird in den Shooterprozess umgeleitet, siehe unden im Prozesshandling
}

//nimmt fuer jeden zustand ein Referenzbild auf, und speichert es in den globalen Variablen
//Im Moment wird nur das Referenzbild mit beiden Flippern unten aufgenommen, also nur case 0 erreicht.
//case 1 ff. sind dazu gedacht, von ausgel�sten Flippern Referenzbilder zu schiessen.
int takeRefShot(IplImage * frame, int& counter){
	switch (counter) {
		case 0 :
			refFlipsDown = frame;
			cerr << "Referenzbild erfolgreich aufgenommen";
			counter++;
			return 0;
		case 1:
			refBothFlipsUp = frame;
			cerr << "Bitte zwei B�lle auf die Flipper legen, damit die Mitte bestimmt werden kann" << endl;
			cerr << "Bei einem beliebigen tastendruck wird die Mitte dann berechnet" << endl;
			counter++;
			return 0;
		case 2:
			// zwei B�lle erkennen, mitte berechnen
			return 364;
	}
}


//Signalhandler f�r das Beenden durch den Nutzer
void SIGINT_handler (int signum)
{
	assert (signum == SIGINT);
	cerr << "Signal interrupt empfangen" << endl;
	cout << "exit" << endl; //
	cerr << "vorwait" << endl;
	wait();
	cerr << "nachwait" << endl;
	exit(0);
}

//Signalhandler f�r abst�rze
void SIGTERM_handler (int signum) {	
	assert (signum == SIGTERM);
	cerr << "SITGTERM empfangen, vermutlich verursacht durch fehlerhaften DV-Stream" << endl;
	cerr << "Bitte versuche erneut, das Programm zu starten" << endl;
	cout << "exit";
	exit(1);
}


// Alte Shoot-Version mit nur 1 Prozess
//Der Flipper soll delay millisekunden oben bleiben und nur die entsprechenden Flips sollen aktiviert werden.
// Zuordnoung ist binaer: 01: rchter flipper, 10, linker flipper, 11 beide
/*void shoot(int delay, int flips){
	switch (flips) {
		case 1:
			system("echo -n 1 >> /dev/ttyUSB0");
			break;
		case 2:
			system("echo -n 2 >> /dev/ttyUSB0");
			break;
		case 3:
			system("echo -n 3 >> /dev/ttyUSB0");
			break;
		default:
			break;
	}
	
	usleep(delay*1000);
	system("echo -n 0 >> /dev/ttyUSB0");
	cerr << "Funktion Shoot ausgeführt" << endl;
}*/



int main(){
	// http://www.cs.wustl.edu/~schmidt/signal-patterns.html
	// http://www.cs.utah.edu/dept/old/texinfo/glibc-manual-0.02/library_21.html
	struct sigaction sa;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;

	/* Register the handler for SIGINT. */
	sa.sa_handler = SIGINT_handler;
	sigaction (SIGINT, &sa, 0);
	
	/* Register the handler for SIGTERM. */
	sa.sa_handler = SIGTERM_handler;
	sigaction (SIGTERM, &sa, 0);

	//Fork, inspiriert nach http://www.gidforums.com/t-3369.html
	pid_t pid;
	int	commpipe[2];		/* This holds the fd for the input & output of the pipe */

	/* Setup communication pipeline first */
	if(pipe(commpipe)){
		fprintf(stderr,"Pipe error!\n");
		exit(1);
	}
	
	//forking
	if( (pid=fork()) == -1){
		fprintf(stderr,"Fork error. Exiting.\n");  /* something went wrong */
		exit(1);        
	}

	if(pid){
		/* A positive (non-negative) PID indicates the parent process */
		dup2(commpipe[1],1);	/* Replace stdout with out side of the pipe */
		close(commpipe[0]);		/* Close unused side of pipe (in side) */
		setvbuf(stdout,(char*)NULL,_IONBF,0);	/* Set non-buffered output on stdout */
		
		//Flipper sagt hallo
		shoot(100, BEIDE);
		shoot(100, LINKS);
		shoot(100, RECHTS);
		shoot(100, LINKS);
		shoot(1000, RECHTS);
		shoot(2000, BEIDE);
		shoot(2000, BEIDE);
		CvMemStorage* cstorage = cvCreateMemStorage(0);
		//Stream laden
		CvCapture* capture = cvCaptureFromFile("/home/jogi/Documents/matura/ballrecognition/camfifo");
		//CvCapture* capture = cvCaptureFromCAM(-1);
		if( !capture ) {
			fprintf( stderr, "ERROR: capture is NULL \n" );
			getchar();
			return -1;
		}
		//Fenster erstellen
		cvNamedWindow( "ball", CV_WINDOW_AUTOSIZE );

		
		int height, width, step, channels, i, j;
		int counter = 0;
		int mittelxkoordinate;
		uchar *data;
		
		//Einen Frame im Voraus abrufen
		// Get one frame
		IplImage* frame = cvQueryFrame( capture );
		if( !frame ) {
			fprintf( stderr, "ERROR: frame is null...\n" );
			getchar();
		}
		
		height = frame->height;
		width     = frame->width;
		step      = frame->widthStep;
		channels  = frame->nChannels;
		data      = (uchar *)frame->imageData;
		IplImage *referenz =  cvCreateImage(cvSize(width, height), 8, 1);
		IplImage *workingcopy = cvCreateImage(cvSize(width, height), 8, 1);
		
			//Warten auf befehl zum Referenzbilder aufnehmen, Esc beginnt die Prozedur
			while (1) {
				// Get one frame
				frame = cvQueryFrame( capture );
				if( !frame ) {
					fprintf( stderr, "ERROR: frame is null... hiho\n" );
					getchar();
					break;
				}
				// Fadenkreuz
				cvCircle(frame, cvPoint(width/2,height/2), 20, cvScalar(0,255,0), 1);
				cvLine(frame, cvPoint(0, height/2), cvPoint(width, height/2), cvScalar(0,255,0), 1);
				cvLine(frame, cvPoint(0, 3*height/4), cvPoint(width, 3*height/4), cvScalar(0,255,0), 1);
				cvLine(frame, cvPoint(width/2, height), cvPoint(width/2, 0), cvScalar(0,255,0), 1);
				cvWaitKey(1);
				cvShowImage( "ball", frame );
				// Do not release the frame!
				
				//If ESC key pressed, take Refshot
				if( (cvWaitKey(10) & 255) == 27 ) {
					takeRefShot(frame, counter);
					cvDestroyAllWindows();
					cerr << "Window closed" << endl;
					break;
				}
			}


			cvCvtColor(refFlipsDown, referenz, CV_BGR2GRAY);
			cvSmooth(referenz, referenz, CV_GAUSSIAN, 17,17 );
			cvWaitKey(250);
		
			//Positionsbestimmung des Balls
			int xkoordinate, ykoordinate, xkoordinatealt, ykoordinatealt, speedx, speedy;
			ykoordinatealt = 0;
			xkoordinatealt = 0;
			xkoordinate = 0;
			ykoordinate = 0;
			counter = 0;
			speedx = 0;
			speedy = 0;
			while (1) {
				//neuer Frame
				frame = cvQueryFrame( capture );
				if( !frame ) {
					fprintf( stderr, "ERROR: frame is null...heyhey\n" );
					break;
				}
				//Position vom Ball bestimmen
				
				

				
				cvCvtColor(frame, workingcopy, CV_BGR2GRAY); //Der abgerufene frame wird in die Variable workingcopy kopiert und dabei in GRaustufen konvertiert
				cvSmooth(workingcopy, workingcopy, CV_GAUSSIAN, 15, 15); //Gausscher weichzeichner
				//Bild mit nicht XOR verkn�pfen
				for (i = 0; i<height; i++) {
					for (j = 0; j<width; j++) {
						if ( j == 0 && i < 10) {
						}
						((uchar *)(workingcopy->imageData + i*workingcopy->widthStep))[j] = ~(((uchar *)(workingcopy->imageData + i*workingcopy->widthStep))[j] ^ ((uchar *)(referenz->imageData + i*referenz->widthStep))[j]);
					}
				}
				
				//Ball suchen
				cvSmooth(workingcopy, workingcopy, CV_GAUSSIAN, 15, 15);

				cvThreshold(workingcopy, workingcopy, 125, 255, CV_THRESH_BINARY);
				

				//cvWaitKey(10);
				
				//cvShowImage("ball", workingcopy);
							
				//Kreise Finden
				CvSeq* circles =  cvHoughCircles( workingcopy, cstorage, CV_HOUGH_GRADIENT, 1, workingcopy->height/50, 12, 10,4,50 );	
			
				//cerr << "Anzahl Gefundener Kreise:" << circles->total << endl;
				//cerr << "Zielkoordinate: " << 375 << endl;
				
				xkoordinatealt = xkoordinate;
				ykoordinatealt = ykoordinate;
				if (circles->total < 10 and circles->total != 0 and counter > 6) {
					xkoordinate = 0;
					ykoordinate = 0;
					//Wenn es weniger als 10 Kreise hat, Bild analysieren.
					for (i=0; i<circles->total; i++) {
					float* p = (float*)cvGetSeqElem(circles, i);
						xkoordinate += cvRound(p[0]);
						ykoordinate += cvRound(p[1]);
					}
					//mttlere koordinaten berechnen
					xkoordinate = xkoordinate/circles->total;
					ykoordinate	= ykoordinate/circles->total;
					cerr << "Koordinaten: " << xkoordinate << "/" << ykoordinate << endl;
					
					//Geschwindigkeit berechnen
					speedx = xkoordinate - xkoordinatealt; //positiv ist bewegung nach rechts
					speedy = ykoordinate - ykoordinatealt; //positiv ist bewegung nach unten
					
					
					
					//eingef�gt, um zu pr�fen, ob das gleiche Bild evtl. mehrmals ausgewertet wird.
					/*if (ykoordinate == ykoordinatealt) {
						cerr << "Y-KOORDINATEN GLEICH!!!!" << endl;
					}
					if (xkoordinate == xkoordinatealt) {
						cerr << "Y-KOORDINATEN GLEICH!!!!" << endl;
					}*/
							
			//Wenn sich die Kugel in der heissen Zone befindet, schiessen, oder wenn sie sich mit der Bewegung in einer heisseren  Zone befinden wird, schiessen
			//horizontale bewegungen werden noch nicht vorausberechnet
					if (ykoordinate > 375 or ykoordinate+speedy > 450) {
						cerr << "ausgel�st" << endl;
						counter = 0;
						if (xkoordinate > width/2 ){
							shoot(100, RECHTS);
						} else {
							shoot(100, LINKS);
						}
						
					}
				} else {
					counter++;
					cerr << counter;
				}
			}
	} //ende des Hauptprozesses
	else{
		/* A zero PID indicates that this is the child process */
		dup2(commpipe[0],0);	/* Replace stdin with the in side of the pipe */
		close(commpipe[1]);		/* Close unused side of pipe (out side) */
		/* Replace the child fork with a new process */
		if(execl("shooter","shooter",NULL) == -1){
			fprintf(stderr,"execl Error!");
			exit(1);
		}
	}
			
	return 0;
}