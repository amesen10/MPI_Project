#include <mpi.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <list>
#include <random>
using namespace std;

#define DEBUG
# define T 4 
void uso(string nombre_prog);

void obt_args(char* argv[], int& numeroPersonas, double& infeccion, double& recuperacion, int& duracion, double& infectadas, int& size);

void iniciar(int *matriz, int& nPersonas, int& nInicialInfectados, int &size)
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

	//aqui va el codigo
	int nPersonas, duracion, size, tics = 0;
	int tInfectadas, tSanas, tCuradas, tInmunes, tMuertas;	//Contadores para las estadísticas de cada tic
	int infectadasT = 0, sanasT = 0, curadasT = 0, inmunesT = 0, muertasT = 0;	//Contadores para las estadísticas finales
	double infeccion, recuperacion, infectadas;
	double tPared;	//t=tiempo
	int veces;	//Será el numero de personas entre la cantidad de procesos

	obt_args(argv, nPersonas, infeccion, recuperacion, duracion, infectadas, size);

	int *matriz = new int[nPersonas * 4];	//puntero al a un arreglo que simula dos dimensiones por donde se van a desplazar las personas
	
	int nInicialInfectados = nPersonas * infectadas;

	if (mid == 0)		//Creación de la matriz en el proceso principal
	{
		cout << "\t MATRIZ INICIAL" << endl << endl;
		iniciar(matriz, nPersonas, nInicialInfectados, size);	//Comienza la simulación
		imprimir(matriz, nPersonas);
	}

	int enfermosRestantes = 0, enfermosTic = 0;
	do
	{
		MPI_Bcast(matriz, nPersonas, MPI_INT, 0, MPI_COMM_WORLD);	//Comparte el espacio (matriz) con todos los procesos
		int *subMatriz = new int[(nPersonas / cnt_proc) *T];
		MPI_Scatter(matriz, nPersonas / cnt_proc * T, MPI_INT, subMatriz, nPersonas / cnt_proc * T, MPI_INT, 0, MPI_COMM_WORLD);		//Se separa el espacio total entre los procesos para realizar las verificaciones correspondientes
		
		//------------------------MOVIMIENTO-------------------------------------
		simulacion(subMatriz, nPersonas, cnt_proc, size);
		MPI_Allgather(subMatriz, (nPersonas / cnt_proc) *T, MPI_INT, matriz, (nPersonas / cnt_proc) *T, MPI_INT, MPI_COMM_WORLD); //Vuelve a reunir la submatriz en matriz
		/*	HASTA ACÁ TODO BIEN :)	*/
		if (mid == 0)		//Creación de la matriz en el proceso principal
		{
			cout << "\t MATRIZ MOVIDA 0" << endl << endl;
			imprimir(matriz, nPersonas);
		}
		free(subMatriz);
		++tics;
	} while (enfermosRestantes!=0);

	MPI_Barrier(MPI_COMM_WORLD);
	free(matriz);		//Liberación de la memoria ocupada
	int n;
	MPI_Allreduce(&tics, &n, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	if (mid == 0)
	{

		//cout << endl << endl << "Time: " << elapsed << "s" << endl;
		cout << endl << "TICS totales: " << tics << endl;
		cin.ignore();
	}
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