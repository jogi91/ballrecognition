//Ansatz: Bild und Referenzbild in Schwarzweiss konvertieren, nichtund laufen lassen, runde Form erkennen
//Das Programm steht bei 0.084s Realzeit, wenn die fenster nicht ge√∂ffnet und die Kreise nicht gezeichnet werden

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
//Globale Variablen fuer die Referenzbilder
IplImage *refFlipsDown = 0;
IplImage *refRightFlipUp = 0;
IplImage *refLeftFlipUp = 0;
IplImage *refBothFlipsUp =0;

void shoot(int delay, int flips){
	cout << flips << delay << endl;
}

//nimmt fuer jeden zustand ein Referenzbild auf, und speichert es in den globalen Variablen
//Return 0 ist Erfolg, alles andere nicht
int takeRefShot(IplImage * frame, int& counter){
	switch (counter) {
		case 0 :
			refFlipsDown = frame;
			cerr << "Referenzbild erfolgreich aufgenommen";
			counter++;
			return 0;
		case 1:
			refBothFlipsUp = frame;
			cerr << "Bitte zwei B‰lle auf die Flipper legen, damit die Mitte bestimmt werden kann" << endl;
			cerr << "Bei einem beliebigen tastendruck wird die Mitte dann berechnet" << endl;
			counter++;
			return 0;
		case 2:
			// zwei B‰lle erkennen, mitte berechnen
			return 364;
	}
}


//handler f¸r das Beenden durch den Nutzer
void SIGINT_handler (int signum)
{
	assert (signum == SIGINT);
	cerr << "Signal interrupt empfangen" << endl;
	cout << "exit" << endl;
	cerr << "vorwait" << endl;
	wait();
	cerr << "nachwait" << endl;
	exit(0);
}

void SIGTERM_handler (int signum) {	
	assert (signum == SIGTERM);
	cerr << "SITGTERM empfangen, vermutlich verursacht durch fehlerhaften DV-Stream" << endl;
	cerr << "Bitte versuche erneut, das Programm zu starten" << endl;
	cout << "exit";
	exit(1);
}

//Der Flipper soll delay millisekunden oben bleiben und nur die entsprechenden Flips sollen aktiviert werden.
// Zuordnoung ist binaer: 01: rchter flipper, 10, linker flipper, 11 beide
// Version mit nur 1 Prozess
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
	cerr << "Funktion Shoot ausgef√ºhrt" << endl;
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

		
		int height, width, step, channels, i, j, k;
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
			while (1) {
				//neuer Frame
				frame = cvQueryFrame( capture );
				if( !frame ) {
					fprintf( stderr, "ERROR: frame is null...heyhey\n" );
					break;
				}
				//Position vom Ball bestimmen
				
				

				
				cvCvtColor(frame, workingcopy, CV_BGR2GRAY);
				cvSmooth(workingcopy, workingcopy, CV_GAUSSIAN, 15, 15);
				//Bild mit nicht XOR verkn√ºpfen
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
							
				
				CvSeq* circles =  cvHoughCircles( workingcopy, cstorage, CV_HOUGH_GRADIENT, 1, workingcopy->height/50, 12, 10,4,50 );	
			
				//cerr << "Anzahl Gefundener Kreise:" << circles->total << endl;
				cerr << "Zielkoordinate: " << 3*height/4+50 << endl;
				int xkoordinate, ykoordinate, counter;
				xkoordinate = 0;
				ykoordinate = 0;
				counter = 0;
				if (circles->total < 10 and circles->total != 0 and counter < 6) {
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
					if (ykoordinate > 375) {
						cerr << "ausgelˆst" << endl;
						counter = 0;
						if (xkoordinate > width/2 ){
							shoot(100, RECHTS);
						} else {
							shoot(100, LINKS);
						}
						
					}
				} else {
					counter++;
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

/*
//Altes Programm, zum Testen, neues steht oben.
int main(){
	int width, height, i, j;
	IplImage *referenz = 0;
	IplImage *mitball = 0;
	
	int px[0], py[0];
	CvMemStorage* cstorage = cvCreateMemStorage(0);
	
	referenz = cvLoadImage("/Users/johannesweinbuch/Documents/Schule/10:11/matura/ballrecognition/samplepic/sample.jpg",0);
	mitball = cvLoadImage("/Users/johannesweinbuch/Documents/Schule/10:11/matura/ballrecognition/samplepic/samplewithball.jpg",0);
	
	width = referenz -> width;
	height = referenz -> height;
	//IplImage* referenzgrau = cvCreateImage(cvSize(width, height), 8, 1);
	//IplImage* mitballgrau = cvCreateImage(cvSize(width, height), 8, 1);

	//cvCvtColor(referenz, referenzgrau, CV_BGR2GRAY);
	//cvCvtColor(mitball, mitballgrau, CV_BGR2GRAY);
	
	//cvThreshold(referenz, referenz, 25, 255, CV_THRESH_BINARY);
	//cvThreshold(mitball, mitball, 25, 255, CV_THRESH_BINARY);
	//cvSmooth(referenz, referenz, CV_GAUSSIAN, 25,25);
	//cvSmooth(mitball, mitball, CV_GAUSSIAN, 25, 25);
	
	cvNamedWindow("referenz", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("mitball", CV_WINDOW_AUTOSIZE);
	
	for (i = 0; i<height; i++) {
		for (j = 0; j<width; j++) {
			if ( j == 0 && i < 10) {
				printf("Pixelwert mitball: %u \n",((uchar *)(mitball->imageData + i*mitball->widthStep))[j]);
				printf("Pixelwert referenz: %u \n",((uchar *)(referenz->imageData + i*referenz->widthStep))[j]);
				printf("Pixelwert neu: %u \n\n",~(((uchar *)(mitball->imageData + i*mitball->widthStep))[j] ^ ((uchar *)(referenz->imageData + i*referenz->widthStep))[j]));
			}
			((uchar *)(mitball->imageData + i*mitball->widthStep))[j] = ~(((uchar *)(mitball->imageData + i*mitball->widthStep))[j] ^ ((uchar *)(referenz->imageData + i*referenz->widthStep))[j]);
		}
	}
	//cvCanny(mitball, mitball, (float)edge_thresh, (float)edge_thresh*3, 5);
	//cvSmooth(mitball, mitball, CV_GAUSSIAN, 25,25);
	cvThreshold(mitball, mitball, 240, 255, CV_THRESH_BINARY);
    cvSmooth(mitball, mitball, CV_GAUSSIAN, 25, 25);

	CvSeq* circles =  cvHoughCircles( mitball, cstorage, CV_HOUGH_GRADIENT, 2, mitball->height/50, 5, 35,30 );	

	printf("Anzahl Gefundener Kreise: %d",circles->total);
	
	for( i = 0; circles->total>=2?i<4:i < circles->total; i++ ){ //just make a filter to limit only <=2 ciecles to draw
		
		float* p = (float*)cvGetSeqElem( circles, i );
		
		cvCircle( referenz, cvPoint(cvRound(p[0]),cvRound(p[1])), 3, CV_RGB(255,255,255), -1, 8, 0 );
		
		cvCircle( referenz, cvPoint(cvRound(p[0]),cvRound(p[1])), cvRound(p[2]), CV_RGB(200,200,200), 1, 8, 0 );
		
		px[i]=cvRound(p[0]); py[i]=cvRound(p[1]);
		
	}
	
	cvShowImage("referenz", referenz);
	cvShowImage("mitball", mitball);
	
	cvWaitKey(0);
	
	cvDestroyWindow("referenz");
	cvDestroyWindow("mitball");
	return 0;
}*/
