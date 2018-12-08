#include <algorithm>
#include <fstream>
#include <iostream>
#include <list>
#include <mpi.h>
#include <random>
#include <string>
#include <vector>

using namespace std;

#define DEBUG
# define T 4 
void uso(string nombre_prog);

void obt_args(char* argv[], int& numeroPersonas, double& infeccion, double& recuperacion, int& duracion, double& infectadas, int& size);

void iniciar(int *matriz, int& nPersonas, int& nInicialInfectados, int& sanasT, int& inmunesT, int &size)
{
	default_random_engine generator;
	uniform_int_distribution <int> distributionXY(0, size - 1);
	uniform_int_distribution <int> distribution12(1, 2);

	//aqui estamos llenando el vector con los enfermos que se tienen como parametro
	for (int iter = 0; iter < nInicialInfectados * T; iter += T)
	{
		*(matriz + iter) = 3;							//Estado 3 (Infectado)
		*(matriz + iter + 1) = 0;						//Dias infectados
		*(matriz + iter + 2) = distributionXY(generator);	//Posición en Eje-X
		*(matriz + iter + 3) = distributionXY(generator);	//Posición en Eje-Y
	}

	//llenamos el vector con la demas cantidad de personas que no estan infectadas, se hace un random que decide si es inmune o sano
	for (int iter = nInicialInfectados * 4; iter<nPersonas * T; iter += T)
	{
		*(matriz + iter) = distribution12(generator);		//Estado 1 (Inmune) o 2 (Sano)
		if (*(matriz + iter) == 2)
			++sanasT;
		else
			++inmunesT;
		*(matriz + iter + 1) = 0;							//Dias infectados
		*(matriz + iter + 2) = distributionXY(generator);	//Posición en Eje-X
		*(matriz + iter + 3) = distributionXY(generator);	//Posición en Eje-Y
	}
}

//metodo que imprime el vector
void imprimir(int const *matriz, int& nPersonas)
{
	for (int iter = 0; iter < nPersonas * T; iter += T)
	{
		cout << *(matriz + iter)
			<< " " << *(matriz + iter + 1)
			<< " X " << *(matriz + iter + 2)
			<< " Y " << *(matriz + iter + 3)
			<< "\t\t";
	}
}

void simulacion(int *subMatriz, int& nPersonas, int& cnt_proc, int& size)
{
	default_random_engine generator;
	uniform_int_distribution <int> distributionXY(0, 1);
	//for (int iter = 0; iter < nPersonas/cnt_proc*T; iter += T)
	for (int iter = 0; iter < (nPersonas / cnt_proc)*T; iter += T)
	{
		//if (*(arreglito + iter) == 3)							//Estado 3 (Infectado)
		if (*(subMatriz + iter) == 3)
			subMatriz[iter + 1] = subMatriz[iter + 1] + 1;	//Dias infectados

															//if (*(arreglito + iter) != 0)	//Si la persona no está muerta (Estado 0) se desplaza por el espacio 
		if (*(subMatriz + iter) != 0)
		{
			if (!distributionXY(generator))	//se moverá un espacio a la derecha
			{
				//*(arreglito + iter + 2) = (*(arreglito + iter + 2) -1); //Movimiento en Eje-X
				*(subMatriz + iter + 2) = (*(subMatriz + iter + 2) - 1); //Movimiento en Eje-X
																		 /*if (*(arreglito + iter + 2) < 0)
																		 *(arreglito + iter + 2) = size - 1;*/
				if (*(subMatriz + iter + 2) < 0)
					*(subMatriz + iter + 2) = size - 1;
			}
			else   //Movimiento hacia la izquierda
				   //*(arreglito + iter + 2) = (*(arreglito + iter + 2) + 1) % size;
				*(subMatriz + iter + 2) = (*(subMatriz + iter + 2) + 1) % size;

			if (!distributionXY(generator))	//se moverá un espacio hacia abajo
			{
				//*(arreglito + iter + 3) = (*(arreglito + iter + 3) -1); //Movimiento en Eje-Y
				*(subMatriz + iter + 3) = (*(subMatriz + iter + 3) - 1); //Movimiento en Eje-Y
																		 /*if (*(arreglito + iter + 3) < 0)
																		 *(arreglito + iter + 3) = size - 1;*/
				if (*(subMatriz + iter + 3) < 0)
					*(subMatriz + iter + 3) = size - 1;
			}
			else	//se moverá un espacio hacia arriba
					//*(arreglito + iter + 3) = (*(arreglito + iter + 3) +1) % size;
				*(subMatriz + iter + 3) = (*(subMatriz + iter + 3) + 1) % size;

		}
	}
	//------------------------------------------------------------------------------------------------------------------

	/*cout << "\n\n MOVIMIENTO\n\n";
	for (int iter = 0; iter < nPersonas/cnt_proc * T; iter += T)
	{
	cout << *(arreglito + iter)
	<< " " << *(arreglito + iter + 1)
	<< " X " << *(arreglito + iter + 2)
	<< " Y " << *(arreglito + iter + 3)
	<< "\t\t";
	}*/
}

void validar(int *matriz, int **mInfectados, int& nPersonas, int& cnt_proc, int& duracion, double& recuperacion, double& infeccion, int& nInfectados, int& tCuradas, int& tMuertas, int& tSanas)
{
	/*NOTA: los enfermosRestantes se pueden estar contando doble, mejor hacer un solo ciclo que busque cuántos enfermos hay, incluso se pdoria hacer en otro método*/
	default_random_engine generator;
	uniform_real_distribution <double> proba(0, 1);

	//int enfermosRestantes=0;
	//enfermosRestantes=0;
	int  posicionEnfermos = 1, contador = 0;
	//for (contador; contador < mid*(nPersonas / cnt_proc)*T+ (nPersonas / cnt_proc)*T; contador += T)
	for (contador; contador < (nPersonas / cnt_proc)*T; contador += T)
	{
		if (*(matriz + contador) == 2)	//Persona sana
		{
			if (mInfectados[*(matriz + contador + 2)][*(matriz + contador + 3)] != 0)	//Si este sano comparte celda con algún enfermo se calcula la probabilidad de infectarlo
			{
				//Se hace el cálculo de la probabilidad y se asigna el nuevo estado en caso de darse el contagio
				if (proba(generator)*mInfectados[*(matriz + contador + 2)][*(matriz + contador + 3)] < infeccion)
				{
					//cout << "\t INFECTA" << endl;
					*(matriz + contador) = 3;
					++nInfectados;
				}
			}
			/*else
				++tSanas;*/
		}
		else
			if (*(matriz + contador) == 3)
			{
				//	2-Verifica si ya es tiempo de morir o de recuperarse
				if (*(matriz + contador + 1) == duracion)
				{
					if (proba(generator) < recuperacion)//Calcular probabilidad de recuperacion o de muerte de acuerdo a  @recuperacion
					{
						*(matriz + contador) = 0;		//Se actualiza el estado a Muerto=0
						//cout << "\t MUERE" << endl;	//Incrementar muertos totales (tMuertas)
						++tMuertas;
					}
					else
					{
						*(matriz + contador) = 1;	//Se convierte a Inmune i.e. se sana
						//cout << "\t CURA" << endl;	//Actualizar el contador de Curadas
						++tCuradas;
					}
				}
			}
	}
}

void llenarMatrizEnfermos(int *matriz, int** infectados, int& nPersonas)
{
	for (int i = 0; i < nPersonas *T; i += T)
	{
		if (*(matriz + i) == 3)
		{
			infectados[*(matriz + i + 2)][*(matriz + i + 3)] = infectados[*(matriz + i + 2)][*(matriz + i + 3)] + 1;
		}
	}
}

//int cuentaInfectados(int *matriz, int** infectados, int& nPersonas, int& cnt_proc, int& mid)
int cuentaInfectados(int *matriz, int& nPersonas, int& cnt_proc, int& mid, int& tInfectadas/*, int& tSanas*/)
{
	//cout << "Infectados" << tInfectadas << "@" << mid << endl;
	int enfermosRestantes = 0;
	//for (int i = mid * (nPersonas / cnt_proc)*T; i < mid * (nPersonas / cnt_proc)*T + (nPersonas / cnt_proc)*T; i += T)
	for (int i = 0; i < (nPersonas / cnt_proc)*T; i += T)
	{
		if (*(matriz + i) == 3)
			++enfermosRestantes;
	}
	//cout << "enfermosRestantes" << enfermosRestantes<<"@"<<mid<<endl;
	return enfermosRestantes;
	
}

int cuentaSanos(int *matriz, int& nPersonas, int& cnt_proc, int& mid)
{
	int sanosRestantes = 0;
	//for (int i = mid * (nPersonas / cnt_proc)*T; i < mid * (nPersonas / cnt_proc)*T + (nPersonas / cnt_proc)*T; i += T)
	for (int i = 0; i < (nPersonas / cnt_proc)*T; i += T)
	{
		if (*(matriz + i) ==2)
		{
			++sanosRestantes;
		}
	}
	//cout << "sanosRestantes" << sanosRestantes << "@" << mid << endl;
	return sanosRestantes;
}

void imprimirEstadisticas(int& const nPersonas,int& const tInfectadas,int& enfermosRestantes, int& const tSanas, int& const tInmunes, int& const tMuertas, int& const tics)
{
	cout << endl << endl << "     ------------------------------ Dia " << tics << " ------------------------------" << endl << endl;
	cout << "\n\t\t\t   Personas infectadas \n\n\t Porcentaje: " << 1.0*tInfectadas / nPersonas << "\t\t\t\t Cantidad actual: " << tInfectadas << endl
		<< "\n\n\t\t\t  Infectadas Restantes \n\n\t Porcentaje: " << 1.0*enfermosRestantes / nPersonas << "\t\t\t\t Cantidad actual: " << enfermosRestantes << endl
		<< "\n\n\t\t\t     Personas Sanas \n\n\t Porcentaje : " << 1.0*tSanas / nPersonas << "\t\t\t\t Cantidad actual: " << tSanas << endl
		<< "\n\n\t\t\t    Personas Curadas \n\n\t Porcentaje : " << 1.0*tInmunes / nPersonas << "\t\t\t\t Cantidad actual: " << tInmunes << endl
		<< "\n\n\t\t\t    Personas Muertas \n\n\t Porcentaje : " << 1.0*tMuertas / nPersonas << "\t\t\t\t Cantidad actual: " << tMuertas << endl << endl;

	ofstream archivo;
	archivo.open("Estadisticas.txt", std::fstream::app);
	archivo << endl << endl
		<< "\t------------------------------ Dia " << tics << " ------------------------------" << endl << endl
		<< "\n\t\t\t  Personas infectadas \n\n\t Porcentaje: " << 1.0*tInfectadas / nPersonas << "\t\t\t\t Cantidad actual: " << tInfectadas << endl
		<< "\n\n\t\t\t  Infectadas Restantes \n\n\t Porcentaje: " << 1.0*enfermosRestantes / nPersonas << "\t\t\t\t Cantidad actual: " << enfermosRestantes << endl
		<< "\n\n\t\t\t     Personas Sanas \n\n\t Porcentaje : " << 1.0*tSanas / nPersonas << "\t\t\t\t Cantidad actual: " << tSanas << endl
		<< "\n\n\t\t\t    Personas Curadas \n\n\t Porcentaje : " << 1.0*tInmunes / nPersonas << "\t\t\t\t Cantidad actual: " << tInmunes << endl
		<< "\n\n\t\t\t    Personas Muertas \n\n\t Porcentaje : " << 1.0*tMuertas / nPersonas << "\t\t\t\t Cantidad actual: " << tMuertas << endl << endl;
	archivo.close();
}

void EstadisticasFinales(int& const nPersonas, int& const infectadasT, int& const tSanas, int& const sanasI, int& const curadasT, int& const inmunesT, int& const muertasT, int& const tics)
{
	cout << "     -------------------------------------------------------------------" << endl << endl;
	cout << "\n\n\n\t\t\t ESTADISTICAS FINALES" << endl << endl;
	cout << "\n\t\t\t  Personas infectadas \n\n\t Porcentaje: " << 1.0*infectadasT / nPersonas << "\t\t\t Cantidad total: " << infectadasT << endl
		<< "\n\n\t\t\t    Sanos Iniciales  \n\n\t Porcentaje : " << 1.0*sanasI / nPersonas << "\t\t\t Cantidad total: " << sanasI << endl
		<< "\n\n\t\t\t    Sanos Restantes \n\n\t Porcentaje : " << 1.0*tSanas / nPersonas << "\t\t\t Cantidad total: " << tSanas << endl
		<< "\n\n\t\t\t   Inmunes Iniciales  \n\n\t Porcentaje : " << 1.0*inmunesT / nPersonas << "\t\t\t Cantidad total: " << inmunesT << endl
		<< "\n\n\t\t\t    Personas Curadas \n\n\t Porcentaje : " << 1.0*curadasT / nPersonas << "\t\t\t Cantidad total: " << curadasT << endl
		<< "\n\n\t\t\t    Personas Muertas \n\n\t Porcentaje : " << 1.0*muertasT / nPersonas << "\t\t\t Cantidad total: " << muertasT << endl << endl;

	ofstream archivo;
	archivo.open("Estadisticas.txt", std::fstream::app);
	archivo << "\t-------------------------------------------------------------------" << endl << endl
		<< "\n\n\t\t\t\t ESTADISTICAS FINALES" << endl << endl
		<< "\n\t\t\t  Personas infectadas \n\n\t Porcentaje: " << 1.0*infectadasT / nPersonas << "\t\t\t Cantidad total: " << infectadasT << endl
		<< "\n\n\t\t\t    Sanos Iniciales \n\n\t Porcentaje : " << 1.0*sanasI / nPersonas << "\t\t\t Cantidad total: " << sanasI << endl
		<< "\n\n\t\t\t    Sanos Restantes \n\n\t Porcentaje : " << 1.0*tSanas / nPersonas << "\t\t\t Cantidad total: " << tSanas << endl
		<< "\n\n\t\t\t   Inmunes Iniciales  \n\n\t Porcentaje : " << 1.0*inmunesT / nPersonas << "\t\t\t Cantidad total: " << inmunesT << endl
		<< "\n\n\t\t\t    Personas Curadas \n\n\t Porcentaje : " << 1.0*curadasT / nPersonas << "\t\t\t Cantidad total: " << curadasT << endl
		<< "\n\n\t\t\t    Personas Muertas \n\n\t Porcentaje : " << 1.0*muertasT / nPersonas << "\t\t\t Cantidad total: " << muertasT << endl << endl;
	archivo.close();
}

int main(int argc, char* argv[]) {
	int mid;
	int cnt_proc;
	MPI_Status mpi_status;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &mid);
	MPI_Comm_size(MPI_COMM_WORLD, &cnt_proc);

#ifdef debug
	if (mid == 0)
		cin.ignore();
	MPI_Barrier(MPI_COMM_WORLD);
#endif

	int nPersonas, duracion, size, tics = 0;
	int tInfectadas=0, tSanas = 0, tCuradas = 0, tInmunes = 0, tMuertas = 0;	//Contadores para las estadísticas de cada tic
	int infectadasT = 0, sanasT = 0, curadasT = 0, inmunesT = 0, muertasT = 0;	//Contadores para las estadísticas finales
	double infeccion, recuperacion, infectadas;
	double local_start, local_finish, local_elapsed, elapsed;
	int inf=0, cur=0, mue=0, san=0;		//Variables temporales para reducir los contadores de cada proceso para las estadísticas
	double tPared=0;	//t=tiempo
	ofstream archivo;

	obt_args(argv, nPersonas, infeccion, recuperacion, duracion, infectadas, size);

	int *matriz = new int[nPersonas * 4];	//puntero al a un arreglo que simula dos dimensiones por donde se van a desplazar las personas
	
	int nInicialInfectados = nPersonas * infectadas;

	if (mid == 0)		//Creación de la matriz en el proceso principal
	{
		//cout << " nInicialInfectados" << nInicialInfectados << endl;
		archivo.open("Estadisticas.txt");
		archivo.close();
		//cout << "\t MATRIZ INICIAL" << endl << endl;
		iniciar(matriz, nPersonas, nInicialInfectados, sanasT, inmunesT, size);	//Comienza la simulación
		//imprimir(matriz, nPersonas);
	}

	int enfermosRestantes = 0, enfermosTic = 0;
	do
	{
		tInfectadas = 0, tCuradas = 0, tSanas = 0, tMuertas = 0;
		int *subMatriz = new int[(nPersonas / cnt_proc) *T];
		int **mInfectados = new int*[size];
		for (int i = 0; i < size; ++i)
		{
			mInfectados[i] = new int[size] {0};
		}
			//Comparte la matriz con la cantidad de infectados por celda
		//iniciar acá el tiempo 
		//if (mid == 0) //La validación(infectar, muerte y sanación se hace con el proceso prinicipal, los resultados luego son compartidos al resto de procesos)
		//{
		//	cout << "Tic" << tics << endl;
		//		//Se recorre de forma paralela el espacio para buscar la cantidad de enfermos en un rango, luego se reducen y se comparten

		//}
		
		local_start = MPI_Wtime();
		MPI_Bcast(matriz, nPersonas, MPI_INT, 0, MPI_COMM_WORLD);	//Comparte el espacio (matriz) con todos los procesos
		MPI_Scatter(matriz, nPersonas / cnt_proc * T, MPI_INT, subMatriz, nPersonas / cnt_proc * T, MPI_INT, 0, MPI_COMM_WORLD);		//Se separa el espacio total entre los procesos para realizar el movimiento de cada una de las personas
		
		llenarMatrizEnfermos(matriz, mInfectados, nPersonas);
		
		/*if(mid==0)
			cout << "enfermosTic" << enfermosTic << endl;
		*/
		//MPI_Bcast(mInfectados, nPersonas, MPI_INT, 0, MPI_COMM_WORLD);	
		//Impresion de la matriz con la cantidad de personas infectadas por celda
		/*if(mid==0)
			for (int i = 0; i < size; ++i)
				for (int j = 0; j < size; +000000000+j)
					cout << mInfectados[i][j] << " ";*/
		//MPI_Scatter(matriz, nPersonas / cnt_proc * T, MPI_INT, subMatriz, nPersonas / cnt_proc * T, MPI_INT, 0, MPI_COMM_WORLD);		//Se separa el espacio total entre los procesos para realizar el movimiento de cada una de las personas
		validar(subMatriz, mInfectados, nPersonas, cnt_proc, duracion, recuperacion, infeccion, tInfectadas, tCuradas, tMuertas, tSanas);
		
		//------------------------MOVIMIENTO-------------------------------------
		simulacion(subMatriz, nPersonas, cnt_proc, size);
		//Se comparte el espacio luego del movimiento hecho por cada uno de los procesos
		//MPI_Allgather(subMatriz, (nPersonas / cnt_proc) *T, MPI_INT, matriz, (nPersonas / cnt_proc) *T, MPI_INT, MPI_COMM_WORLD); //Vuelve a reunir la submatriz en matriz
		
		//MPI_Allgather(subMatriz, (nPersonas / cnt_proc) *T, MPI_INT, matriz, (nPersonas / cnt_proc) *T, MPI_INT, MPI_COMM_WORLD); //Vuelve a reunir la submatriz en matriz

		enfermosTic = cuentaInfectados(subMatriz, nPersonas, cnt_proc, mid, tInfectadas);
		tSanas = cuentaSanos(subMatriz, nPersonas, cnt_proc, mid);
		MPI_Barrier(MPI_COMM_WORLD);
		
		infectadasT += tInfectadas;
		curadasT += tCuradas;
		muertasT += tMuertas;
		MPI_Allreduce(&enfermosTic, &enfermosRestantes, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);	//Reducción para saber llevar el control de cuándo se estabiliza la infección
		
		
		MPI_Reduce(&tSanas, &san, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
		MPI_Reduce(&tInfectadas, &inf, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
		MPI_Reduce(&tCuradas, &cur, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
		MPI_Reduce(&tMuertas, &mue, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
		
		MPI_Gather(subMatriz, (nPersonas / cnt_proc) *T, MPI_INT, matriz, (nPersonas / cnt_proc) *T, MPI_INT, 0, MPI_COMM_WORLD); //Vuelve a reunir la submatriz en matriz
		
		local_finish = MPI_Wtime();
		local_elapsed = local_finish - local_start;
		MPI_Reduce(&local_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
		
		
		if (mid == 0)	//Se imprimen las estadísticas correspondientes a cada tic
		{
			tPared += elapsed;
			imprimirEstadisticas(nPersonas, inf, enfermosRestantes, san, cur, mue, tics);
		}

		free(subMatriz);
		for (int i = 0; i < size; ++i)
			delete[] mInfectados[i];
		delete[] mInfectados;
		++tics;
	} while (enfermosRestantes!=0);
	inf = 0, cur = 0, mue = 0;
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Reduce(&infectadasT, &inf, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(&curadasT, &cur, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(&muertasT, &mue, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	if (mid == 0)
	{
		//acá va la impresión final (usar las variable sque terminan con T)
		inf += nInicialInfectados;
		EstadisticasFinales(nPersonas, inf, san/*para los sanos restantes*/, sanasT, cur, inmunesT, mue, tics);

		//cout << "\t MATRIZ FINAL " << tics << endl << endl;
		//imprimir(matriz, nPersonas);
		cout<< endl << "\t  Tics totales: " << tics << endl
			<< endl << "\t  Tiempo-Pared: " << tPared << "s" << endl	//y lo de tiempo barrera
			<< endl << "\t  Tiempo-Pared/Tic: " << tPared / tics << "s" << endl;
		archivo.open("Estadisticas.txt", std::fstream::app);
		archivo << "\t-------------------------------------------------------------------" << endl << endl
			<< endl << "\t  Tics totales: " << tics << endl
			<< endl << "\t  Tiempo-Pared: " << tPared << "s" << endl	//y lo de tiempo barrera
			<< endl << "\t  Tiempo-Pared/Tic: " << tPared / tics << "s" << endl;
		archivo.close();
		cin.ignore();
	}
	free(matriz);		//Liberación de la memoria ocupada
	MPI_Finalize();
	return 0;
}

void uso(string nombre_prog) {
	cerr << nombre_prog.c_str() << " secuencia de parametros de entrada " << endl;
	exit(0);
}
void obt_args(char* argv[], int& numeroPersonas, double& infeccion, double& recuperacion, int& duracion, double& infectadas, int& size) {

	numeroPersonas = strtol(argv[1], NULL, 10);
	infeccion = stod(argv[2]);
	recuperacion = stod(argv[3]);
	duracion = strtol(argv[4], NULL, 10);
	infectadas = stod(argv[5]);
	size = strtol(argv[6], NULL, 10);

	if (numeroPersonas < 0)
	{
		do {
			cout << "\t Numero de personas invalido, digite otra cifra [0, 10.000.000]: ";
			cin >> numeroPersonas;
		} while (numeroPersonas < 0);
	}
	if (infeccion < 0 || infeccion>1)
	{
		do {
			cout << "\t Probabilidad infecciosa invalida, digite otra cifra [0, 1]: ";
			cin >> infeccion;
		} while (infeccion < 0 || infeccion>1);
	}
	if (recuperacion < 0 || recuperacion>1)
	{
		do {
			cout << "\t Probabilidad de recuperacion invalida, digite otra cifra [0, 1]: ";
			cin >> recuperacion;
		} while (recuperacion < 0 || recuperacion>1);
	}
	if (duracion < 5 || duracion>50)
	{
		do {
			cout << "\t Duracion infecciosa maxima invalida, digite otra cifra [0, 50]: ";
			cin >> duracion;
		} while (duracion < 5 || duracion>50);
	}
	if (infectadas < 0 || infectadas>1)
	{
		do {
			cout << "\t Porcentaje personas incialmente infectadas invalido, digite otra cifra [0, 10]: ";
			cin >> infectadas;
		} while (infectadas < 0 || infectadas>1);
	}
	if (size < 1 || size>3)
	{
		do {
			cout << "\t Tamano invalido, digite otra cifra: \n"
				<< "\t    1) 100x100" << endl
				<< "\t    2) 500x500" << endl
				<< "\t    3) 1000x1000" << endl;
			cin >> size;
		} while (size < 1 || size>3);
	}
	switch (size)
	{
	case 1: size = 6/*100*/; break;
	case 2: size = 500; break;
	case 3: size = 1000; break;
	}
}