// Example code that runs Pythia8 and creates ProIO file
// It requires extra card file with the settings
// S.Chekanov (ANL)

#include <map>
#include <limits>       
#include <stdio.h>
#include <time.h>
#pragma GCC diagnostic ignored "-pedantic"
#pragma GCC diagnostic ignored "-Wshadow"
#include <proio/writer.h>
#include <proio/event.h>
#include <proio/model/mc.pb.h>
#include <CLHEP/Random/MTwistEngine.h>//mercenne twister high rng
#include <CLHEP/Units/PhysicalConstants.h>//pi 

namespace model=proio::model::mc;

#include "Pythia8/Pythia.h"
using namespace Pythia8;

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while(std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	return split(s, delim, elems);
}


string getEnvVar( std::string const & key ) {
	char * val = getenv( key.c_str() );
	return val == NULL ? std::string("") : std::string(val);
}



int main(int argc, char* argv[]) {


	// Check that correct number of command-line arguments
	if (argc != 3) {
		cerr << " Unexpected number of command-line arguments. \n You are"
		<< " expected to provide one input config and one output ProMC file name. \n"
		<< " Program stopped! " << endl;
		return 1;
	}

	cout << "HepSim:  Pythia8 Input Configuration =" << argv[1] << endl;
	cout << "HepSim:  ProsIOC Output =" << argv[2] << endl;

	auto writer=new proio::Writer( argv[2] );



	// Generator. Process selection. Tevatron initialization. Histogram.
	Pythia pythia;



	/////////// read config files ////////////////////
	string sets="";
	string sets1="";
	bool   apply_slim=true;


	int Ntot=0 ; // total number of events
	vector<string> configs;
	string events;
	ifstream myfile;
	myfile.open(argv[1], ios::in);
	if (!myfile) {
		cerr << "Can't open input file:  " << argv[1] << endl;
		exit(1);
	} else {
		string line;
		while(getline(myfile,line))
		{
			//the following line trims white space from the beginning of the string
			//line.erase(line.begin(), find_if(line.begin(), line.end(), not1(ptr_fun<int, int>(isspace))));
			if(line[0] == '#') continue;
			if (line.length()<3) continue;
			string tmp=string(line);
			// no empty spaces inside string
			std::string::iterator end_pos = std::remove(tmp.begin(), tmp.end(), ' ');
			tmp.erase(end_pos, tmp.end());
			bool special=false;
			int found1=tmp.find("EventsNumber");
		if (found1!=(int)std::string::npos) {events=tmp; special=true;}
			int found2=tmp.find("ApplyParticleSlim=on");
			if (found2!=(int)std::string::npos) {apply_slim=true; special=true;}
			int found3=tmp.find("ApplyParticleSlim=off");
			if (found3!=(int)std::string::npos) {apply_slim=false; special=true;}
			if (!special)  {sets1=sets1+tmp+"; "; pythia.readString(line); }
			configs.push_back(line);
		}
		myfile.close();
		vector<string> readnum=split(events,'=');
		Ntot= atoi(readnum[1].c_str());
		cout << "Reading events. " << events << " Total number is=" << Ntot<< endl;
		for (unsigned int i=0; i<configs.size(); i++) {
			cout << ".. input ="+configs[i] << endl;
			sets=sets+configs[i]+";";
		}
	} // end else
	pythia.init();

	pythia.settings.listChanged(); // Show changed settings
	double versionNumber = pythia.settings.parm("Pythia:versionNumber");
	pythia.particleData.listChanged(); // Show changed particle data
	std::stringstream s;
	s << versionNumber;
	string version=s.str();

	//time
	time_t time_raw_format;
	time ( &time_raw_format );

	// Use the range 0.01 MeV to 20 TeV using varints (integers)
	// if particle in GeV, we mutiple it by kEV, to get 0.01 MeV =1 unit
	// const double kEV=1000*100;
	// for 13 TeV, increase the precision
	double kEV=1000*100;
	double slimPT=0.3;
	// special run
	double kL=1000;

	// for 100 TeV, reduce the precision
	// const double kEV=1000*10;
	// set units dynamically
	// e+e- 250, 500 GeV
	if (pythia.info.eCM() <1000) {
		kEV=1000*1000;
		slimPT=0.1;
		kL=10000;
	}

	if (pythia.info.eCM() <20000 &&  pythia.info.eCM()>=1000) {
		kEV=1000*100;
		slimPT=0.3;
		kL=1000;

	}

	if (pythia.info.eCM() >=20000) { // larger energy, i.e. 100 TeV
		kEV=1000*10;
		slimPT=0.3;
		kL=1000;
	}

	writer->PushMetadata("info:eCM",  (std::to_string(pythia.info.eCM())).c_str() );
	writer->PushMetadata("info:idA",  (std::to_string(pythia.info.idA())).c_str() );
	writer->PushMetadata("info:idB",  (std::to_string(pythia.info.idB())).c_str() );
	writer->PushMetadata("info:varint_energy",  (std::to_string(kEV)).c_str() );
	writer->PushMetadata("info:varint_length",  (std::to_string(kL)).c_str() );

	int ntot=0;
	for (int n = 0; n < Ntot; n++) {
		if (!pythia.next()) continue;
		// if (n < 1) {pythia.info.list(); pythia.event.list();}
		// Loop over particles in event. Find last Z0 copy. Fill its pT.

		if (n<=10 &&
		                ((n<=100 && (n%10) == 0)) ||
		                ((n<=1000 && (n%100) == 0))  ||
		                ((n>=1000 && (n%1000) == 0)) ) {
			cout << "No events= " << n << " passed"  << endl; };

		// If failure because reached end of file then exit event loop.
		if (pythia.info.atEndOfFile()) {
			cout << " Aborted since reached end of Les Houches Event File\n";
			break;
		}


		auto event=new proio::Event();


		auto params = new proio::model::mc::MCParameters();
		auto pythia_params =new proio::model::mc::Pythia8Parameters();

		// general MC parameters
		params->set_number(n);
		params->set_processid(pythia.info.code());
		params->set_weight(pythia.info.weight());

		// Pythia8 parameters
		pythia_params->set_pt_hat(pythia.info.pTHat());
		pythia_params->set_alpha_em(pythia.info.alphaEM());
		pythia_params->set_alpha_s(pythia.info.alphaS());
		pythia_params->set_scale_q_fac(pythia.info.QFac());
		pythia_params->set_weight_sum(pythia.info.weightSum());
		pythia_params->set_merging_weight(pythia.info.mergingWeight());
		pythia_params->set_x1(pythia.info.x1pdf());
		pythia_params->set_x2(pythia.info.x2pdf());
		pythia_params->set_id1(pythia.info.id1pdf());
		pythia_params->set_id2(pythia.info.id2pdf());

		auto pa = new proio::model::mc::VarintPackedParticles();


		for (int i =0; i<pythia.event.size(); i++) {

			int pdgid=pythia.event[i].id();
			int status=pythia.event[i].statusHepMC();

			if (apply_slim) {
				int take=false;
				if (i<9) take=true;                               // first original
				if (abs(pdgid)==5 ||  abs(pdgid)==6 )             take=true; // top and b
				if (abs(pdgid)>10 && abs(pdgid)<17)               take=true; // leptons etc.
				if (abs(pdgid)>22 && abs(pdgid)<37)               take=true; // exotic
				if (status ==1 && pythia.event[i].pT()>slimPT)    take=true; // final state
				if (take==false) continue;
			}


			double ee=pythia.event[i].e()*kEV;
			double px=pythia.event[i].px()*kEV;
			double py=pythia.event[i].py()*kEV;
			double pz=pythia.event[i].pz()*kEV;
			double mm=pythia.event[i].m()*kEV;
			double xx=pythia.event[i].xProd()*kL;
			double yy=pythia.event[i].yProd()*kL;
			double zz=pythia.event[i].zProd()*kL;
			double tt=pythia.event[i].tProd()*kL;
			double charge= pythia.event[i].charge();

			pa->add_pdg( pdgid );
			pa->add_status(  status );
			pa->add_px( (int)px );
			pa->add_py( (int)py );
			pa->add_pz( (int)pz  );
			pa->add_mass( (int)mm );
			pa->add_energy( (int)ee );
			pa->add_parent1( pythia.event[i].mother1()  );
			pa->add_parent2( pythia.event[i].mother2()  );
			pa->add_child1( pythia.event[i].daughter1()  );
			pa->add_child2( pythia.event[i].daughter2()   );
			pa->add_barcode( 0 ); // dummy
			pa->add_weight( 1 ); // dummy
			pa->add_charge( (int)(3*charge)  ); // dummy
			pa->add_id( i  );
			pa->add_x( (int)xx  );
			pa->add_y( (int)yy  );
			pa->add_z( (int)zz  );
			pa->add_t( (int)tt  );

		} // end loop over particles





		ntot++;
		if (ntot>Ntot) break;

		event->AddEntry(params,"MCParameters");
		event->AddEntry(pythia_params,"Pythia8Parameters");
		event->AddEntry(pa,"VarintPackedParticles");
		writer->Push(event);

	} // end of loop over events


	// To check which changes have actually taken effect
	pythia.settings.listChanged();
	// pythia.particleData.listChanged();
	pythia.particleData.list(25);
	// ParticleDataTable::listAll()
	// ParticleDataTable::list(25);


	pythia.stat();



	// write to empty event
	auto event=new proio::Event();

	// Output histograms
	double sigmapb = pythia.info.sigmaGen() * 1.0E9;
	double sigmapb_err = pythia.info.sigmaErr() * 1.0E9;
	double lumi=(Ntot/sigmapb);

	writer->PushMetadata("meta:description", (string("PYTHIA-")+version+"; "+sets).c_str());
	writer->PushMetadata("meta:creation_time", ctime(&time_raw_format));
	writer->PushMetadata("meta:cross_section_pb",  (std::to_string( sigmapb )).c_str() );
	writer->PushMetadata("meta:cross_section_pb_err",  (std::to_string( sigmapb_err )).c_str() );
	writer->PushMetadata("meta:luminosity_inv_pb",  (std::to_string( lumi )).c_str() );
	writer->PushMetadata("meta:events_accepted",  (std::to_string( pythia.info.nAccepted() )).c_str() );
	writer->PushMetadata("meta:events_ntried",  (std::to_string( pythia.info.nTried() )).c_str() );
	writer->PushMetadata("meta:events",  (std::to_string( Ntot )).c_str() );
	writer->PushMetadata("meta:events_requested",  (std::to_string( Ntot )).c_str() );



	ifstream ifile("logfile.txt");
	if (ifile) {
		cout << "ProIO: Adding logfile.txt" << endl;
		std::ifstream t("logfile.txt");
		std::stringstream buffer;
		buffer << t.rdbuf();
		writer->PushMetadata("meta:logfile",  (buffer.str()).c_str() );
	} else {
		cout << "Warning: Missing logfile.txt" << endl;
	}

	// empty event
	writer->Push(event);


	cout << "== Run statistics: " << endl;
	cout << "== Cross section    =" <<  sigmapb << " +- " << sigmapb_err << " pb" << endl;
	cout << "== Generated Events =" <<  Ntot << endl;
	lumi=(Ntot/sigmapb)/1000;
	cout << "== Luminosity       =" <<  lumi  << " fb-1" << endl;
	cout << "\n\n";

	delete writer;


	return 0;
}



