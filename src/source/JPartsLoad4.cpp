//HEAD_DSPH
/*
 <DUALSPHYSICS>  Copyright (c) 2017 by Dr Jose M. Dominguez et al. (see http://dual.sphysics.org/index.php/developers/). 

 EPHYSLAB Environmental Physics Laboratory, Universidade de Vigo, Ourense, Spain.
 School of Mechanical, Aerospace and Civil Engineering, University of Manchester, Manchester, U.K.

 This file is part of DualSPHysics. 

 DualSPHysics is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License 
 as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.
 
 DualSPHysics is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details. 

 You should have received a copy of the GNU Lesser General Public License along with DualSPHysics. If not, see <http://www.gnu.org/licenses/>. 
*/

/// \file JPartsLoad4.cpp \brief Implements the class \ref JPartsLoad4.

#include "JPartsLoad4.h"
#include "Functions.h"
#include "JPartDataBi4.h"
#include "JRadixSort.h"
#include "JXml.h"
#include <cmath>
#include <climits>
#include <cfloat>

using namespace std;

//##############################################################################
//# JPartsLoad4
//##############################################################################
//==============================================================================
/// Constructor.
//==============================================================================
JPartsLoad4::JPartsLoad4(bool useomp):UseOmp(useomp){
  ClassName="JPartsLoad4";
  Idp = NULL; Pos = NULL; VelRhop = NULL; Mass = NULL; Qf = NULL;
  Reset();
}

//==============================================================================
/// Destructor.
//==============================================================================
JPartsLoad4::~JPartsLoad4(){
  DestructorActive=true;
  AllocMemory(0);
}

//==============================================================================
/// Initialisation of variables.
//==============================================================================
void JPartsLoad4::Reset(){
  Simulate2D=false;
  Simulate2DPosY=0;
  CaseNp=CaseNfixed=CaseNmoving=CaseNfloat=CaseNfluid=0;
  PeriMode=PERI_None;
  PeriXinc=PeriYinc=PeriZinc=TDouble3(0);
  MapSize=false;
  MapPosMin=MapPosMax=TDouble3(0);
  PartBegin=0;
  PartBeginTimeStep=0;
  AllocMemory(0);
}

//==============================================================================
/// Resizes space for particle data.
/// Redimensiona espacio para datos de las particulas.
//==============================================================================
void JPartsLoad4::AllocMemory(unsigned count){
  Count=count;
  delete[] Idp;      Idp=NULL; 
  delete[] Pos;      Pos=NULL; 
  delete[] VelRhop;  VelRhop=NULL;
  delete[] Mass;  Mass = NULL;
  delete[] Qf;  Qf = NULL;
  if(Count){
    try{
      Idp=new unsigned[Count];
      Pos=new tdouble3[Count];
      VelRhop=new tfloat4[Count];
	  Mass = new float[Count];
	  Qf = new tsymatrix3f[Count];
    }
    catch(const std::bad_alloc){
      RunException("AllocMemory","Could not allocate the requested memory.");
    }
  } 
}

//==============================================================================
/// Returns the reserved memory in CPU.
/// Devuelve la memoria reservada en CPU.
//==============================================================================
llong JPartsLoad4::GetAllocMemory()const{  
  llong s=0;
  //-Allocated in AllocMemory().
  if(Idp)s+=sizeof(unsigned)*Count;
  if(Pos)s+=sizeof(tdouble3)*Count;
  if(VelRhop)s+=sizeof(tfloat4)*Count;
  return(s);
}

//==============================================================================
/// Sorts values according to vsort[].
/// Ordena valores segun vsort[].
//==============================================================================
template<typename T> T* JPartsLoad4::SortParticles(const unsigned *vsort,unsigned count,T *v)const{
  T* v2=new T[count];
  for(unsigned p=0;p<count;p++)v2[p]=v[vsort[p]];
  delete[] v;
  return(v2);
}

//==============================================================================
/// Sorts values according to Idp[].
/// Ordena particulas por Idp[].
//==============================================================================
void JPartsLoad4::SortParticles(){
  //-Checks order. | Comprueba orden.
  bool sorted=true;
  for(unsigned p=1;p<Count&&sorted;p++)sorted=(Idp[p-1]<Idp[p]);
  if(!sorted){
    //-Sorts points according to id. | Ordena puntos segun id.
    JRadixSort rs(UseOmp);
    rs.Sort(true,Count,Idp);
    rs.SortData(Count,Pos,Pos);
    rs.SortData(Count,VelRhop,VelRhop);
  }
}

//==============================================================================
/// It loads particles of bi4 file and it orders them by Id.
/// Carga particulas de fichero bi4 y las ordena por Id.
//==============================================================================
void JPartsLoad4::LoadParticles(const std::string &casedir,const std::string &casename,unsigned partbegin,const std::string &casedirbegin){
  const char met[]="LoadParticles";
  Reset();
  PartBegin=partbegin;
  JPartDataBi4 pd;
  //-Loads file piece_0 and obtains configuration.
  //-Carga fichero piece_0 y obtiene configuracion.
  const string dir=fun::GetDirWithSlash(!PartBegin? casedir: casedirbegin);
  if(!PartBegin){
    const string file1=dir+JPartDataBi4::GetFileNameCase(casename,0,1);
    if(fun::FileExists(file1))pd.LoadFileCase(dir,casename,0,1);
    else if(fun::FileExists(dir+JPartDataBi4::GetFileNameCase(casename,0,2)))pd.LoadFileCase(dir,casename,0,2);
    else RunException(met,"File of the particles was not found.",file1);
  }
  else{
    const string file1=dir+JPartDataBi4::GetFileNamePart(PartBegin,0,1);
    if(fun::FileExists(file1))pd.LoadFilePart(dir,PartBegin,0,1);
    else if(fun::FileExists(dir+JPartDataBi4::GetFileNamePart(PartBegin,0,2)))pd.LoadFilePart(dir,PartBegin,0,2);
    else RunException(met,"File of the particles was not found.",file1);
  }
  
  //-Obtains configuration. | Obtiene configuracion.
  PartBeginTimeStep=(!PartBegin? 0: pd.Get_TimeStep());
  Npiece=pd.GetNpiece();
  Simulate2D=pd.Get_Data2d();
  Simulate2DPosY=(Simulate2D? pd.Get_Data2dPosY(): 0);
  NpDynamic=pd.Get_NpDynamic();
  PartBeginTotalNp=(NpDynamic? pd.Get_NpTotal(): 0);
  CaseNp=pd.Get_CaseNp();
  CaseNfixed=pd.Get_CaseNfixed();
  CaseNmoving=pd.Get_CaseNmoving();
  CaseNfloat=pd.Get_CaseNfloat();
  CaseNfluid=pd.Get_CaseNfluid();
  JPartDataBi4::TpPeri peri=pd.Get_PeriActive();
  if(peri==JPartDataBi4::PERI_None)PeriMode=PERI_None;
  else if(peri==JPartDataBi4::PERI_X)PeriMode=PERI_X;
  else if(peri==JPartDataBi4::PERI_Y)PeriMode=PERI_Y;
  else if(peri==JPartDataBi4::PERI_Z)PeriMode=PERI_Z;
  else if(peri==JPartDataBi4::PERI_XY)PeriMode=PERI_XY;
  else if(peri==JPartDataBi4::PERI_XZ)PeriMode=PERI_XZ;
  else if(peri==JPartDataBi4::PERI_YZ)PeriMode=PERI_YZ;
  else if(peri==JPartDataBi4::PERI_Unknown)PeriMode=PERI_Unknown;
  else RunException(met,"Periodic configuration is invalid.");
  PeriXinc=pd.Get_PeriXinc();
  PeriYinc=pd.Get_PeriYinc();
  PeriZinc=pd.Get_PeriZinc();
  MapPosMin=pd.Get_MapPosMin();
  MapPosMax=pd.Get_MapPosMax();
  MapSize=(MapPosMin!=MapPosMax);
  CasePosMin=pd.Get_CasePosMin();
  CasePosMax=pd.Get_CasePosMax();
  const bool possingle=pd.Get_PosSimple();
  if(!pd.Get_IdpSimple())RunException(met,"Only Idp (32 bits) is valid at the moment.");

  //-Calculates number of particles. | Calcula numero de particulas.
  unsigned sizetot=pd.Get_Npok();
  for(unsigned piece=1;piece<Npiece;piece++){
    JPartDataBi4 pd2;
    if(!PartBegin)pd2.LoadFileCase(dir,casename,piece,Npiece);
    else pd2.LoadFilePart(dir,PartBegin,piece,Npiece);
    sizetot+=pd.Get_Npok();
  }
  //-Allocates memory.
  AllocMemory(sizetot);
  //-Loads particles.
  {
    unsigned ntot=0;
    unsigned auxsize=0;
    tfloat3 *auxf3=NULL;
    float *auxf=NULL;
    for(unsigned piece=0;piece<Npiece;piece++){
      if(piece){
        if(!PartBegin)pd.LoadFileCase(dir,casename,piece,Npiece);
        else pd.LoadFilePart(dir,PartBegin,piece,Npiece);
      }
      const unsigned npok=pd.Get_Npok();
      if(npok){
        if(auxsize<npok){
          auxsize=npok;
          delete[] auxf3; auxf3=NULL;
          delete[] auxf;  auxf=NULL;
          auxf3=new tfloat3[auxsize];
          auxf=new float[auxsize];

        }
        if(possingle){
          pd.Get_Pos(npok,auxf3);
          for(unsigned p=0;p<npok;p++)Pos[ntot+p]=ToTDouble3(auxf3[p]);
        }
        else pd.Get_Posd(npok,Pos+ntot);
        pd.Get_Idp(npok,Idp+ntot);
        pd.Get_Vel(npok,auxf3);
        pd.Get_Rhop(npok,auxf);
		for (unsigned p = 0; p<npok; p++)VelRhop[ntot + p] = TFloat4(auxf3[p].x, auxf3[p].y, auxf3[p].z, auxf[p]);
		for (unsigned p = 0; p < npok; p++) Mass[ntot + p] = (float)pd.Get_MassFluid();
		const double Dp = pd.Get_Dp();
		for (unsigned p = 0; p < npok; p++) {
			Qf[ntot + p] = TSymatrix3f(
				4 / float(pow(Dp, 2)), 0, 0, 4 / float(pow(Dp, 2)), 0, 4 / float(pow(Dp, 2)));
		}
      }
      ntot+=npok;
    }
    delete[] auxf3; auxf3=NULL;
    delete[] auxf;  auxf=NULL;
  }

  //-In simulations 2D, if PosY is invalid then calculates starting from position of particles.
  if(Simulate2DPosY==DBL_MAX){
    if(!sizetot)RunException(met,"Number of particles is invalid to calculates Y in 2D simulations.");
    Simulate2DPosY=Pos[0].y;
  }
  //-Sorts particles according to Id. | Ordena particulas por Id.
  SortParticles();
}

//==============================================================================
/// It loads particles of bi4 file and it orders them by Id.
/// Carga particulas de fichero bi4 y las ordena por Id.
//==============================================================================
void JPartsLoad4::LoadParticles_T(const std::string &casedir, const std::string &casename, unsigned partbegin, const std::string &casedirbegin) {
	const char met[] = "LoadParticles";
	Reset();
	PartBegin = partbegin;
	JPartDataBi4 pd;
	//-Loads file piece_0 and obtains configuration.
	//-Carga fichero piece_0 y obtiene configuracion.
	const string dir = fun::GetDirWithSlash(!PartBegin ? casedir : casedirbegin);
	if (!PartBegin) {
		const string file1 = dir + JPartDataBi4::GetFileNameCase(casename, 0, 1);
		if (fun::FileExists(file1))pd.LoadFileCase(dir, casename, 0, 1);
		else if (fun::FileExists(dir + JPartDataBi4::GetFileNameCase(casename, 0, 2)))pd.LoadFileCase(dir, casename, 0, 2);
		else RunException(met, "File of the particles was not found.", file1);
	}
	else {
		const string file1 = dir + JPartDataBi4::GetFileNamePart(PartBegin, 0, 1);
		if (fun::FileExists(file1))pd.LoadFilePart(dir, PartBegin, 0, 1);
		else if (fun::FileExists(dir + JPartDataBi4::GetFileNamePart(PartBegin, 0, 2)))pd.LoadFilePart(dir, PartBegin, 0, 2);
		else RunException(met, "File of the particles was not found.", file1);
	}

	//-Obtains configuration. | Obtiene configuracion.
	PartBeginTimeStep = (!PartBegin ? 0 : pd.Get_TimeStep());
	Npiece = pd.GetNpiece();
	Simulate2D = pd.Get_Data2d();
	Simulate2DPosY = (Simulate2D ? pd.Get_Data2dPosY() : 0);
	NpDynamic = pd.Get_NpDynamic();
	PartBeginTotalNp = (NpDynamic ? pd.Get_NpTotal() : 0);
	CaseNp = pd.Get_CaseNp();
	CaseNfixed = pd.Get_CaseNfixed();
	CaseNmoving = pd.Get_CaseNmoving();
	CaseNfloat = pd.Get_CaseNfloat();
	CaseNfluid = pd.Get_CaseNfluid();
	JPartDataBi4::TpPeri peri = pd.Get_PeriActive();
	if (peri == JPartDataBi4::PERI_None)PeriMode = PERI_None;
	else if (peri == JPartDataBi4::PERI_X)PeriMode = PERI_X;
	else if (peri == JPartDataBi4::PERI_Y)PeriMode = PERI_Y;
	else if (peri == JPartDataBi4::PERI_Z)PeriMode = PERI_Z;
	else if (peri == JPartDataBi4::PERI_XY)PeriMode = PERI_XY;
	else if (peri == JPartDataBi4::PERI_XZ)PeriMode = PERI_XZ;
	else if (peri == JPartDataBi4::PERI_YZ)PeriMode = PERI_YZ;
	else if (peri == JPartDataBi4::PERI_Unknown)PeriMode = PERI_Unknown;
	else RunException(met, "Periodic configuration is invalid.");
	PeriXinc = pd.Get_PeriXinc();
	PeriYinc = pd.Get_PeriYinc();
	PeriZinc = pd.Get_PeriZinc();
	MapPosMin = pd.Get_MapPosMin();
	MapPosMax = pd.Get_MapPosMax();
	MapSize = (MapPosMin != MapPosMax);
	CasePosMin = pd.Get_CasePosMin();
	CasePosMax = pd.Get_CasePosMax();
	const bool possingle = pd.Get_PosSimple();
	if (!pd.Get_IdpSimple())RunException(met, "Only Idp (32 bits) is valid at the moment.");

	//-Calculates number of particles. | Calcula numero de particulas.
	unsigned sizetot = pd.Get_Npok();
	for (unsigned piece = 1; piece<Npiece; piece++) {
		JPartDataBi4 pd2;
		if (!PartBegin)pd2.LoadFileCase(dir, casename, piece, Npiece);
		else pd2.LoadFilePart(dir, PartBegin, piece, Npiece);
		sizetot += pd.Get_Npok();
	}

	//-Allocates memory.
	AllocMemory(sizetot);
	//-Loads particles.
	{
		unsigned ntot = 0;
		unsigned auxsize = 0;
		tfloat3 *auxf3 = NULL;
		float *auxf = NULL;
		float* auxfbis = NULL;
		tsymatrix3f* auxf6= NULL;
		for (unsigned piece = 0; piece<Npiece; piece++) {
			if (piece) {
				if (!PartBegin)pd.LoadFileCase(dir, casename, piece, Npiece);
				else pd.LoadFilePart(dir, PartBegin, piece, Npiece);
			}
			const unsigned npok = pd.Get_Npok();
			if (npok) {
				if (auxsize<npok) {
					auxsize = npok;
					delete[] auxf3; auxf3 = NULL;
					delete[] auxf;  auxf = NULL;
					delete[] auxfbis;  auxfbis = NULL;
					delete[] auxf6;  auxf6 = NULL;
					auxf3 = new tfloat3[auxsize];
					auxf = new float[auxsize];
					auxfbis = new float[auxsize];
					auxf6 = new tsymatrix3f[auxsize];

				}
				if (possingle) {
					pd.Get_Pos(npok, auxf3);
					for (unsigned p = 0; p<npok; p++)Pos[ntot + p] = ToTDouble3(auxf3[p]);
				}
				else pd.Get_Posd(npok, Pos + ntot);
				pd.Get_Idp(npok, Idp + ntot);
				pd.Get_Vel(npok, auxf3);
				pd.Get_Rhop(npok, auxf);
				pd.Get_Mass(npok, auxfbis);
				pd.Get_Qf(npok, auxf6);
				for (unsigned p = 0; p < npok; p++) {
					VelRhop[ntot + p] = TFloat4(auxf3[p].x, auxf3[p].y, auxf3[p].z, auxf[p]);
					Mass[p] = auxfbis[p];
					Qf[p] = auxf6[p];
				}
			}
			ntot += npok;
		}
		delete[] auxf3; auxf3 = NULL;
		delete[] auxf;  auxf = NULL;
		delete[] auxfbis;  auxfbis = NULL;
	}

	//-In simulations 2D, if PosY is invalid then calculates starting from position of particles.
	if (Simulate2DPosY == DBL_MAX) {
		if (!sizetot)RunException(met, "Number of particles is invalid to calculates Y in 2D simulations.");
		Simulate2DPosY = Pos[0].y;
	}
	//-Sorts particles according to Id. | Ordena particulas por Id.
	SortParticles();
}

//==============================================================================
/// It loads particles of bi4 file and it orders them by Id, from 
/// Gencase and Unique particule. Matthias version 2019
//==============================================================================
void JPartsLoad4::LoadParticles_Mixed_M(const std::string& casedir, const std::string& casename, unsigned partbegin, const std::string& casedirbegin) {
	const char met[] = "LoadParticles_Mixed_M";
	//#Mixed #loadparticles
	Reset();
	PartBegin = partbegin;
	JPartDataBi4 pd;
	//JPartDataBi4 pd_unique;

	//-Loads file piece_0 and obtains configuration.
	//-Carga fichero piece_0 y obtiene configuracion.
	const string dir = fun::GetDirWithSlash(!PartBegin ? casedir : casedirbegin);
	if (!PartBegin) {
		const string file1 = dir + JPartDataBi4::GetFileNameCase(casename, 0, 1);
		if (fun::FileExists(file1))pd.LoadFileCase(dir, casename, 0, 1);
		else if (fun::FileExists(dir + JPartDataBi4::GetFileNameCase(casename, 0, 2)))pd.LoadFileCase(dir, casename, 0, 2);
		else RunException(met, "File of the particles was not found.", file1);
	}
	else {
		const string file1 = dir + JPartDataBi4::GetFileNamePart(PartBegin, 0, 1);
		if (fun::FileExists(file1))pd.LoadFilePart(dir, PartBegin, 0, 1);
		else if (fun::FileExists(dir + JPartDataBi4::GetFileNamePart(PartBegin, 0, 2)))pd.LoadFilePart(dir, PartBegin, 0, 2);
		else RunException(met, "File of the particles was not found.", file1);
	}

	//-Obtains configuration. | Obtiene configuracion.
	PartBeginTimeStep = (!PartBegin ? 0 : pd.Get_TimeStep());
	Npiece = pd.GetNpiece();
	Simulate2D = pd.Get_Data2d();
	Simulate2DPosY = (Simulate2D ? pd.Get_Data2dPosY() : 0);
	NpDynamic = pd.Get_NpDynamic();
	PartBeginTotalNp = (NpDynamic ? pd.Get_NpTotal() : 0);
	CaseNp = pd.Get_CaseNp();
	CaseNfixed = pd.Get_CaseNfixed();
	CaseNmoving = pd.Get_CaseNmoving();

	// Ptc unique
	CaseNfloat = pd.Get_CaseNfloat();
	CaseNfluid = pd.Get_CaseNfluid()+1;
	JPartDataBi4::TpPeri peri = pd.Get_PeriActive();
	if (peri == JPartDataBi4::PERI_None)PeriMode = PERI_None;
	else if (peri == JPartDataBi4::PERI_X)PeriMode = PERI_X;
	else if (peri == JPartDataBi4::PERI_Y)PeriMode = PERI_Y;
	else if (peri == JPartDataBi4::PERI_Z)PeriMode = PERI_Z;
	else if (peri == JPartDataBi4::PERI_XY)PeriMode = PERI_XY;
	else if (peri == JPartDataBi4::PERI_XZ)PeriMode = PERI_XZ;
	else if (peri == JPartDataBi4::PERI_YZ)PeriMode = PERI_YZ;
	else if (peri == JPartDataBi4::PERI_Unknown)PeriMode = PERI_Unknown;
	else RunException(met, "Periodic configuration is invalid.");
	PeriXinc = pd.Get_PeriXinc();
	PeriYinc = pd.Get_PeriYinc();
	PeriZinc = pd.Get_PeriZinc();
	MapPosMin = pd.Get_MapPosMin();
	MapPosMax = pd.Get_MapPosMax();
	MapSize = (MapPosMin != MapPosMax);
	CasePosMin = pd.Get_CasePosMin();
	CasePosMax = pd.Get_CasePosMax();
	const bool possingle = pd.Get_PosSimple();
	if (!pd.Get_IdpSimple())RunException(met, "Only Idp (32 bits) is valid at the moment.");

	//-Calculates number of particles. | Calcula numero de particulas.
	unsigned sizetot = pd.Get_Npok()+1; //unique
	printf("N = %d\n", sizetot);
	//unsigned sizetot = pd.Get_Npok();
	for (unsigned piece = 1; piece < Npiece; piece++) {
		JPartDataBi4 pd2;
		if (!PartBegin)pd2.LoadFileCase(dir, casename, piece, Npiece);
		else pd2.LoadFilePart(dir, PartBegin, piece, Npiece);
		sizetot += pd.Get_Npok();
	}

	//-Allocates memory.
	AllocMemory(sizetot);

	//-Loads particles.
	{
		unsigned ntot = 0;
		unsigned auxsize = 0;
		tfloat3* auxf3 = NULL;
		float* auxf = NULL;
		for (unsigned piece = 0; piece < Npiece; piece++) {
			if (piece) {
				if (!PartBegin)pd.LoadFileCase(dir, casename, piece, Npiece);
				else pd.LoadFilePart(dir, PartBegin, piece, Npiece);
			}
			//const unsigned npok = pd.Get_Npok(); Unique
			const unsigned npok = pd.Get_Npok();
			if (npok) {
				if (auxsize < npok) {
					auxsize = npok;
					delete[] auxf3; auxf3 = NULL;
					delete[] auxf;  auxf = NULL;
					auxf3 = new tfloat3[auxsize];
					auxf = new float[auxsize];

				}
				if (possingle) {
					pd.Get_Pos(npok, auxf3); 
					for (unsigned p = 0; p < npok; p++)Pos[ntot + p] = ToTDouble3(auxf3[p]);
					//Unique
					//for (unsigned p = 0; p < npok; p++)Pos[ntot + p] = ToTDouble3({ 0,0,0 });
				}
				else pd.Get_Posd(npok, Pos + ntot);
				//else for (unsigned p = 0; p < npok; p++)Pos[ntot + p] = ToTDouble3({ 0,0,0 });
				pd.Get_Idp(npok, Idp + ntot);
				//for (unsigned p = 0; p < npok; p++)Idp[ntot + p] = 0;
				pd.Get_Vel(npok, auxf3);
				pd.Get_Rhop(npok, auxf);
				//for (unsigned p = 0; p < npok; p++)VelRhop[ntot + p] = TFloat4(0.0f, 0.0f, 0.0f, 1000.0f);
			}
			ntot += npok;
		}

		// Unique particle generation
		Idp[ntot] = Idp[ntot - 1] + 1;
		if (possingle) Pos[ntot] = ToTDouble3({ 0,0,0 });
		else Pos[ntot] = ToTDouble3({ 0.0, 0.0, 0.0 });
		VelRhop[ntot] = TFloat4(0.0f, 0.0f, 0.0f, 1000.0f);
		printf("Unic %d %.8f %.8f\n", Idp[ntot], Pos[ntot].x, VelRhop[ntot].x);
		CaseNp++;

		delete[] auxf3; auxf3 = NULL;
		delete[] auxf;  auxf = NULL;
	}
	

	//-In simulations 2D, if PosY is invalid then calculates starting from position of particles.
	if (Simulate2DPosY == DBL_MAX) {
		if (!sizetot)RunException(met, "Number of particles is invalid to calculates Y in 2D simulations.");
		Simulate2DPosY = Pos[0].y;
	}
	//-Sorts particles according to Id. | Ordena particulas por Id.
	SortParticles();
}


//==============================================================================
/// It loads particles of bi4 file and it orders them by Id, from 
/// Gencase and Unique particule. Matthias version 2019
//==============================================================================
void JPartsLoad4::LoadParticles_Mixed2_M(const std::string& casedir, const std::string& casename, unsigned partbegin
	, const std::string& casedirbegin, const std::string& datacasename) {
	const char met[] = "LoadParticles_Mixed_M";
	//#Csv #read
	Reset();
	PartBegin = partbegin;
	JPartDataBi4 pd;
	//#printf
	printf("Call of Mixed Loadparticle2\n");
	JPartDataBi4 pd_csv;

	//-Loads file piece_0 and obtains configuration.
	//-Carga fichero piece_0 y obtiene configuracion.
	const string dir = fun::GetDirWithSlash(!PartBegin ? casedir : casedirbegin);
	if (!PartBegin) {
		const string file1 = dir + JPartDataBi4::GetFileNameCase(casename, 0, 1);
		if (fun::FileExists(file1))pd.LoadFileCase(dir, casename, 0, 1);
		else if (fun::FileExists(dir + JPartDataBi4::GetFileNameCase(casename, 0, 2)))pd.LoadFileCase(dir, casename, 0, 2);
		else RunException(met, "File of the particles was not found.", file1);
	}
	else {
		const string file1 = dir + JPartDataBi4::GetFileNamePart(PartBegin, 0, 1);
		if (fun::FileExists(file1))pd.LoadFilePart(dir, PartBegin, 0, 1);
		else if (fun::FileExists(dir + JPartDataBi4::GetFileNamePart(PartBegin, 0, 2)))pd.LoadFilePart(dir, PartBegin, 0, 2);
		else RunException(met, "File of the particles was not found.", file1);
	}

	// Read Csv and load root particles
	//ReadCsv_M(JPartDataBi4 pd_csv); Remains to be defined rhopZero, Gamma, Dp, h, m. There is a volume field 
	//pd_csv.ReadCsv_M(pd.Get_Npok(), pd.Get_PosSimple());
	pd_csv.ReadCsv_M(pd.Get_Npok(), pd.Get_PosSimple());
	

	//> Here update the .bi4 ?

	//-Obtains configuration. | Obtiene configuracion.
	PartBeginTimeStep = (!PartBegin ? 0 : pd.Get_TimeStep());
	Npiece = pd.GetNpiece();
	Simulate2D = pd.Get_Data2d();
	Simulate2DPosY = (Simulate2D ? pd.Get_Data2dPosY() : 0);
	NpDynamic = pd.Get_NpDynamic();
	PartBeginTotalNp = (NpDynamic ? pd.Get_NpTotal() : 0);
	// Take incot account csv particles
	CaseNp = pd.Get_CaseNp()+pd_csv.Get_CaseNp();
	CaseNfixed = pd.Get_CaseNfixed();
	CaseNmoving = pd.Get_CaseNmoving();

	//ptc unique
	CaseNfloat = pd.Get_CaseNfloat();
	CaseNfluid = pd.Get_CaseNfluid() + pd_csv.Get_CaseNfluid();
	JPartDataBi4::TpPeri peri = pd.Get_PeriActive();
	if (peri == JPartDataBi4::PERI_None)PeriMode = PERI_None;
	else if (peri == JPartDataBi4::PERI_X)PeriMode = PERI_X;
	else if (peri == JPartDataBi4::PERI_Y)PeriMode = PERI_Y;
	else if (peri == JPartDataBi4::PERI_Z)PeriMode = PERI_Z;
	else if (peri == JPartDataBi4::PERI_XY)PeriMode = PERI_XY;
	else if (peri == JPartDataBi4::PERI_XZ)PeriMode = PERI_XZ;
	else if (peri == JPartDataBi4::PERI_YZ)PeriMode = PERI_YZ;
	else if (peri == JPartDataBi4::PERI_Unknown)PeriMode = PERI_Unknown;
	else RunException(met, "Periodic configuration is invalid.");
	PeriXinc = pd.Get_PeriXinc();
	PeriYinc = pd.Get_PeriYinc();
	PeriZinc = pd.Get_PeriZinc();
	MapPosMin = pd.Get_MapPosMin();
	MapPosMax = pd.Get_MapPosMax();
	MapSize = (MapPosMin != MapPosMax);
	CasePosMin = pd.Get_CasePosMin();
	CasePosMax = pd.Get_CasePosMax();
	const bool possingle = pd.Get_PosSimple();
	if (!pd.Get_IdpSimple())RunException(met, "Only Idp (32 bits) is valid at the moment.");

	//-Calculates number of particles. | Calcula numero de particulas.
	unsigned sizetot = pd.Get_Npok() + pd_csv.Get_Npok(); // w Csv ptcs
	for (unsigned piece = 1; piece < Npiece; piece++) {
		JPartDataBi4 pd2;
		if (!PartBegin)pd2.LoadFileCase(dir, casename, piece, Npiece);
		else pd2.LoadFilePart(dir, PartBegin, piece, Npiece);
		sizetot += pd.Get_Npok();
	}

	//-Allocates memory.
	AllocMemory(sizetot);

	// Load cst massfluid and rhop0
	printf("Constants reading\n");
	float RhopZero_csv, MassFluid_csv;
	JXml xml; xml.LoadFile(casedir + casename + ".xml");
	((xml.GetNode("case.execution.constants.rhop0", false))->ToElement())->QueryFloatAttribute("value", &RhopZero_csv);
	((xml.GetNode("case.execution.constants.massfluid", false))->ToElement())->QueryFloatAttribute("value", &MassFluid_csv);
	printf("RhopZero %.8f Massfluid %.8f\n", RhopZero_csv, MassFluid_csv);

	//-Loads particles.
	{
		unsigned ntot = 0;
		unsigned auxsize = 0;
		tfloat3* auxf3 = NULL;
		float* auxf = NULL;
		for (unsigned piece = 0; piece < Npiece; piece++) {
			if (piece) {
				if (!PartBegin)pd.LoadFileCase(dir, casename, piece, Npiece);
				else pd.LoadFilePart(dir, PartBegin, piece, Npiece);
			}

			const unsigned npok = pd.Get_Npok();
			if (npok) {
				if (auxsize < npok) {
					auxsize = npok;
					delete[] auxf3; auxf3 = NULL;
					delete[] auxf;  auxf = NULL;
					auxf3 = new tfloat3[auxsize];
					auxf = new float[auxsize];

				}
				if (possingle) {
					pd.Get_Pos(npok, auxf3);
					for (unsigned p = 0; p < npok; p++)Pos[ntot + p] = ToTDouble3(auxf3[p]);
				}
				else pd.Get_Posd(npok, Pos + ntot);
				pd.Get_Idp(npok, Idp + ntot);
				pd.Get_Vel(npok, auxf3);
				//pd.Get_Rhop(npok, auxf);
				for (unsigned p = 0; p < npok; p++) VelRhop[ntot + p] = TFloat4(auxf3[p].x, auxf3[p].y, auxf3[p].z, RhopZero_csv);
				for (unsigned p = 0; p < npok; p++) Mass[ntot + p] = MassFluid_csv;
			}
			ntot += npok;
		}

		const unsigned npok_csv = pd_csv.Get_Npok();
		auxsize = npok_csv;
		delete[] auxf3; auxf3 = NULL;
		delete[] auxf;  auxf = NULL;
		auxf3 = new tfloat3[auxsize];
		auxf = new float[auxsize];
		if (possingle) {
			pd_csv.Get_Pos(npok_csv, auxf3);
			for (unsigned p = 0; p < npok_csv; p++)Pos[ntot + p] = ToTDouble3(auxf3[p]);
		}
		else pd_csv.Get_Posd(npok_csv, Pos + ntot);
		pd_csv.Get_Idp(npok_csv, Idp + ntot);
		pd_csv.Get_Vel(npok_csv, auxf3);
		for (unsigned p = 0; p < npok_csv; p++) {
			VelRhop[ntot + p] = TFloat4(auxf3[p].x, auxf3[p].y, auxf3[p].z, RhopZero_csv); 
		}
		pd_csv.Get_Mass(npok_csv, Mass + ntot);
		ntot += npok_csv;

		delete[] auxf3; auxf3 = NULL;
		delete[] auxf;  auxf = NULL;
	}


	//-In simulations 2D, if PosY is invalid then calculates starting from position of particles.
	if (Simulate2DPosY == DBL_MAX) {
		if (!sizetot)RunException(met, "Number of particles is invalid to calculates Y in 2D simulations.");
		Simulate2DPosY = Pos[0].y;
	}
	//-Sorts particles according to Id. | Ordena particulas por Id.
	SortParticles();
}

//==============================================================================
/// It loads particles of bi4 file and it orders them by Id, from 
/// Gencase and Unique particule. 
/// Matthias version 2019.2: include custom file name for loading
//==============================================================================
void JPartsLoad4::LoadParticles_Mixed3_M(const std::string& casedir, const std::string& casename, unsigned partbegin
	, const std::string& casedirbegin, const std::string& datacasename, const std::string& datacsvname) {
	const char met[] = "LoadParticles_Mixed_M";
	//#Csv #read
	Reset();
	PartBegin = partbegin;
	JPartDataBi4 pd;
	JPartDataBi4 pd_csv;

	//-Loads file piece_0 and obtains configuration.
	//-Carga fichero piece_0 y obtiene configuracion.
	const string dir = fun::GetDirWithSlash(!PartBegin ? casedir : casedirbegin);
	if (!PartBegin) {
		const string file1 = dir + JPartDataBi4::GetFileNameCase(casename, 0, 1);
		if (fun::FileExists(file1))pd.LoadFileCase(dir, casename, 0, 1);
		else if (fun::FileExists(dir + JPartDataBi4::GetFileNameCase(casename, 0, 2)))pd.LoadFileCase(dir, casename, 0, 2);
		else RunException(met, "File of the particles was not found.", file1);
	}
	else {
		const string file1 = dir + JPartDataBi4::GetFileNamePart(PartBegin, 0, 1);
		if (fun::FileExists(file1))pd.LoadFilePart(dir, PartBegin, 0, 1);
		else if (fun::FileExists(dir + JPartDataBi4::GetFileNamePart(PartBegin, 0, 2)))pd.LoadFilePart(dir, PartBegin, 0, 2);
		else RunException(met, "File of the particles was not found.", file1);
	}

	// Read Csv and load root particles
	//ReadCsv_M(JPartDataBi4 pd_csv); Remains to be defined rhopZero, Gamma, Dp, h, m. There is a volume field 
	//pd_csv.ReadCsv_M(pd.Get_Npok(), pd.Get_PosSimple(), datacsvname);
	pd_csv.ReadCsv_Ellipsoid_M(pd.Get_Npok(), pd.Get_PosSimple(), datacsvname);


	//> Here update the .bi4 ?

	//-Obtains configuration. | Obtiene configuracion.
	PartBeginTimeStep = (!PartBegin ? 0 : pd.Get_TimeStep());
	Npiece = pd.GetNpiece();
	Simulate2D = pd.Get_Data2d();
	Simulate2DPosY = (Simulate2D ? pd.Get_Data2dPosY() : 0);
	NpDynamic = pd.Get_NpDynamic();
	PartBeginTotalNp = (NpDynamic ? pd.Get_NpTotal() : 0);
	// Take incot account csv particles
	CaseNp = pd.Get_CaseNp() + pd_csv.Get_CaseNp();
	CaseNfixed = pd.Get_CaseNfixed();
	CaseNmoving = pd.Get_CaseNmoving();

	//ptc unique
	CaseNfloat = pd.Get_CaseNfloat();
	CaseNfluid = pd.Get_CaseNfluid() + pd_csv.Get_CaseNfluid();
	JPartDataBi4::TpPeri peri = pd.Get_PeriActive();
	if (peri == JPartDataBi4::PERI_None)PeriMode = PERI_None;
	else if (peri == JPartDataBi4::PERI_X)PeriMode = PERI_X;
	else if (peri == JPartDataBi4::PERI_Y)PeriMode = PERI_Y;
	else if (peri == JPartDataBi4::PERI_Z)PeriMode = PERI_Z;
	else if (peri == JPartDataBi4::PERI_XY)PeriMode = PERI_XY;
	else if (peri == JPartDataBi4::PERI_XZ)PeriMode = PERI_XZ;
	else if (peri == JPartDataBi4::PERI_YZ)PeriMode = PERI_YZ;
	else if (peri == JPartDataBi4::PERI_Unknown)PeriMode = PERI_Unknown;
	else RunException(met, "Periodic configuration is invalid.");
	PeriXinc = pd.Get_PeriXinc();
	PeriYinc = pd.Get_PeriYinc();
	PeriZinc = pd.Get_PeriZinc();
	MapPosMin = pd.Get_MapPosMin();
	MapPosMax = pd.Get_MapPosMax();
	MapSize = (MapPosMin != MapPosMax);
	CasePosMin = pd.Get_CasePosMin();
	CasePosMax = pd.Get_CasePosMax();
	const bool possingle = pd.Get_PosSimple();
	if (!pd.Get_IdpSimple())RunException(met, "Only Idp (32 bits) is valid at the moment.");

	//-Calculates number of particles. | Calcula numero de particulas.
	unsigned sizetot = pd.Get_Npok() + pd_csv.Get_Npok(); // w Csv ptcs
	for (unsigned piece = 1; piece < Npiece; piece++) {
		JPartDataBi4 pd2;
		if (!PartBegin)pd2.LoadFileCase(dir, casename, piece, Npiece);
		else pd2.LoadFilePart(dir, PartBegin, piece, Npiece);
		sizetot += pd.Get_Npok();
	}

	//-Allocates memory.
	AllocMemory(sizetot);

	// Load cst massfluid and rhop0
	printf("Constants reading\n");
	//float RhopZero_csv, MassFluid_csv;
	JXml xml; xml.LoadFile(casedir + casename + ".xml");
	/*((xml.GetNode("case.execution.constants.rhop0", false))->ToElement())->QueryFloatAttribute("value", &RhopZero_csv);
	((xml.GetNode("case.execution.constants.massfluid", false))->ToElement())->QueryFloatAttribute("value", &MassFluid_csv);
	printf("RhopZero %.8f Massfluid %.8f\n", RhopZero_csv, MassFluid_csv);*/

	//-Loads particles.
	{
		unsigned ntot = 0;
		unsigned auxsize = 0;
		tfloat3* auxf3 = NULL;
		float* auxf = NULL;
		float* auxfbis = NULL;
		tsymatrix3f* auxf6 = NULL;

		for (unsigned piece = 0; piece < Npiece; piece++) {
			if (piece) {
				if (!PartBegin)pd.LoadFileCase(dir, casename, piece, Npiece);
				else pd.LoadFilePart(dir, PartBegin, piece, Npiece);
			}
			const unsigned npok = pd.Get_Npok();
			if (npok) {
				if (auxsize < npok) {
					auxsize = npok;
					delete[] auxf3; auxf3 = NULL;
					delete[] auxf;  auxf = NULL;
					delete[] auxfbis;  auxfbis = NULL;
					delete[] auxf6;  auxf6 = NULL;
					auxf3 = new tfloat3[auxsize];
					auxf = new float[auxsize];
					auxfbis = new float[auxsize];
					auxf6 = new tsymatrix3f[auxsize];

				}
				if (possingle) {
					pd.Get_Pos(npok, auxf3);
					for (unsigned p = 0; p < npok; p++)Pos[ntot + p] = ToTDouble3(auxf3[p]);
				}
				else pd.Get_Posd(npok, Pos + ntot);
				pd.Get_Idp(npok, Idp + ntot);
				pd.Get_Vel(npok, auxf3);
				//pd.Get_Rhop(npok, auxf);
				// Dp = pow(MassFluid_csv / RhopZero_csv, 1.0f / 3.0f);
				const double Dp = pd.Get_Dp();
				for (unsigned p = 0; p < npok; p++) VelRhop[ntot + p] = TFloat4(auxf3[p].x, auxf3[p].y, auxf3[p].z, (float) pd.Get_Rhop0());
				for (unsigned p = 0; p < npok; p++) Mass[ntot + p] = (float) pd.Get_MassFluid();
				for (unsigned p = 0; p < npok; p++) {
					Qf[ntot + p] = TSymatrix3f(
						4 / float(pow(Dp, 2)), 0, 0, 4 / float(pow(Dp, 2)), 0, 4 / float(pow(Dp, 2)));
				}
			}
			ntot += npok;
		}

		const unsigned npok_csv = pd_csv.Get_Npok();
		auxsize = npok_csv;
		delete[] auxf3; auxf3 = NULL;
		delete[] auxf;  auxf = NULL;
		delete[] auxfbis;  auxfbis = NULL;
		delete[] auxf6;  auxf6 = NULL;
		auxf3 = new tfloat3[auxsize];
		auxf = new float[auxsize];
		auxfbis = new float[auxsize];
		auxf6 = new tsymatrix3f[auxsize];
		if (possingle) {
			pd_csv.Get_Pos(npok_csv, auxf3);
			for (unsigned p = 0; p < npok_csv; p++)Pos[ntot + p] = ToTDouble3(auxf3[p]);
		}
		else pd_csv.Get_Posd(npok_csv, Pos + ntot);
		pd_csv.Get_Idp(npok_csv, Idp + ntot);
		pd_csv.Get_Mass(npok_csv, Mass + ntot);
		pd_csv.Get_Vel(npok_csv, auxf3);
		pd_csv.Get_Qf(npok_csv, auxf6);
		for (unsigned p = 0; p < npok_csv; p++) {
			VelRhop[ntot + p] = TFloat4(auxf3[p].x, auxf3[p].y, auxf3[p].z, (float) pd.Get_Rhop0());
			Qf[ntot + p] = auxf6[p];
		}
		ntot += npok_csv;

		delete[] auxf3; auxf3 = NULL;
		delete[] auxf;  auxf = NULL;
		delete[] auxfbis;  auxfbis = NULL;
		delete[] auxf6;  auxf6 = NULL;
	}

	//-In simulations 2D, if PosY is invalid then calculates starting from position of particles.
	if (Simulate2DPosY == DBL_MAX) {
		if (!sizetot)RunException(met, "Number of particles is invalid to calculates Y in 2D simulations.");
		Simulate2DPosY = Pos[0].y;
	}
	//-Sorts particles according to Id. | Ordena particulas por Id.
	SortParticles();
}

//==============================================================================
/// Check validity of loaded configuration or throw exception.
/// Comprueba validez de la configuracion cargada o lanza excepcion.
//==============================================================================
void JPartsLoad4::CheckConfig(ullong casenp,ullong casenfixed,ullong casenmoving,ullong casenfloat,ullong casenfluid,bool perix,bool periy,bool periz)const
{
  const char met[]="CheckConfig";
  CheckConfig(casenp,casenfixed,casenmoving,casenfloat,casenfluid);
  //-Obtains periodic mode and compares with loaded file.
  //-Obtiene modo periodico y compara con el cargado del fichero.
  TpPeri tperi=PERI_None;
  if(perix&&periy)tperi=PERI_XY;
  else if(perix&&periz)tperi=PERI_XZ;
  else if(periy&&periz)tperi=PERI_YZ;
  else if(perix)tperi=PERI_X;
  else if(periy)tperi=PERI_Y;
  else if(periz)tperi=PERI_Z;
  if(tperi!=PeriMode && PeriMode!=PERI_Unknown)RunException(met,"Data file does not match the periodic configuration of the case.");
}

//==============================================================================
/// Check validity of loaded configuration or throw exception.
/// Comprueba validez de la configuracion cargada o lanza excepcion.
//==============================================================================
void JPartsLoad4::CheckConfig(ullong casenp,ullong casenfixed,ullong casenmoving,ullong casenfloat,ullong casenfluid)const
{
  const char met[]="CheckConfig";
  if(casenp!=CaseNp || casenfixed!=CaseNfixed || casenmoving!=CaseNmoving || casenfloat!=CaseNfloat || casenfluid!=CaseNfluid)RunException(met,"Data file does not match the configuration of the case.");
}

//==============================================================================
/// Removes boundary particles.
/// Elimina particulas de contorno.
//==============================================================================
void JPartsLoad4::RemoveBoundary(){
  const unsigned casenbound=unsigned(CaseNp-CaseNfluid);
  //-Calculate number of boundary particles. 
  unsigned nbound=0;
  for(;nbound<Count && Idp[nbound]<casenbound;nbound++);
  //-Saves old pointers and allocates new memory.
  unsigned count0=Count;
  unsigned *idp0=Idp;        Idp=NULL;
  tdouble3 *pos0=Pos;        Pos=NULL;
  tfloat4 *velrhop0=VelRhop; VelRhop=NULL;
  AllocMemory(count0-nbound);
  //-Copies data in new pointers.
  memcpy(Idp,idp0+nbound,sizeof(unsigned)*Count);
  memcpy(Pos,pos0+nbound,sizeof(tdouble3)*Count);
  memcpy(VelRhop,velrhop0+nbound,sizeof(tfloat4)*Count);
  //-Frees old pointers.
  delete[] idp0;      idp0=NULL; 
  delete[] pos0;      pos0=NULL; 
  delete[] velrhop0;  velrhop0=NULL; 
}

//==============================================================================
/// Returns the limits of the map and if they are not valid throws exception.
/// Devuelve los limites del mapa y si no son validos genera excepcion.
//==============================================================================
void JPartsLoad4::GetMapSize(tdouble3 &mapmin,tdouble3 &mapmax)const{
  if(!MapSizeLoaded())RunException("GetMapSize","The MapSize information is invalid.");
  mapmin=MapPosMin; mapmax=MapPosMax;
}

//==============================================================================
/// Calculates limits of the loaded particles.
/// Calcula limites de las particulas cargadas.
//==============================================================================
void JPartsLoad4::CalculateCasePos(){
  if(!PartBegin)RunException("CalculateCasePos","The limits of the initial case cannot be calculated from a file PART.");
  tdouble3 pmin=TDouble3(DBL_MAX),pmax=TDouble3(-DBL_MAX);
  //-Calculates minimum and maximum position. 
  //-Calcula posicion minima y maxima. 
  for(unsigned p=0;p<Count;p++){
    const tdouble3 ps=Pos[p];
    if(pmin.x>ps.x)pmin.x=ps.x;
    if(pmin.y>ps.y)pmin.y=ps.y;
    if(pmin.z>ps.z)pmin.z=ps.z;
    if(pmax.x<ps.x)pmax.x=ps.x;
    if(pmax.y<ps.y)pmax.y=ps.y;
    if(pmax.z<ps.z)pmax.z=ps.z;
  }
  CasePosMin=pmin; CasePosMax=pmax;
}

//==============================================================================
/// Calculates and returns particle limits with the indicated border.
/// Calcula y devuelve limites de particulas con el borde indicado.
//==============================================================================
void JPartsLoad4::CalculeLimits(double border,double borderperi,bool perix,bool periy,bool periz,tdouble3 &mapmin,tdouble3 &mapmax){
  if(CasePosMin==CasePosMax)CalculateCasePos();
  tdouble3 bor=TDouble3(border);
  if(perix)bor.x=borderperi;
  if(periy)bor.y=borderperi;
  if(periz)bor.z=borderperi;
  mapmin=CasePosMin-bor;
  mapmax=CasePosMax+bor;
}




