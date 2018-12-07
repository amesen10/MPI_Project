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

void validar(int *matriz, int& nPersonas, int& cnt_proc, int& duracion, double& recuperacion, double& infeccion, int& nInfectados, int& tCuradas, int& tMuertas, int& tSanas)
{
	/*NOTA: los enfermosRestantes se pueden estar contando doble, mejor hacer un solo ciclo que busque cuántos enfermos hay, incluso se pdoria hacer en otro método*/
	default_random_engine generator;
	uniform_real_distribution <double> proba(0, 1);
	//int enfermosRestantes=0;
	//enfermosRestantes=0;
	int  posicionEnfermos = 1, contador = 0;
	for (contador; contador < nPersonas*T; contador += T)
	{
		if (*(matriz + contador) == 2)	//Persona sana
			++tSanas;
		else
		{
			posicionEnfermos = 1;
			//Si está infectado:
			if (*(matriz + contador) == 3)
			{	//	1-Busca infectar a los que pueda
				for (int i = 0; i < nPersonas*T; i += T)		//Busca los enfermos en la misma posición que el enfermo actual
				{
					if (*(matriz + contador + 2) == *(matriz + i + 2) &&
						*(matriz + contador + 3) == *(matriz + i + 3) &&
						*(matriz + i) == 3)
					{
						++posicionEnfermos;
					}

				}

				for (int i = 0; i < nPersonas*T; i += T)
				{
					if (*(matriz + contador + 2) == *(matriz + i + 2) &&	//Si el enfermo está en la misma celda que el enfermo y además está sano
						*(matriz + contador + 3) == *(matriz + i + 3) &&
						*(matriz + i) == 2)
					{
						//Se hace el cálculo de la probabilidad y se asigna el nuevo estado en caso de darse el contagio
						if (proba(generator)*posicionEnfermos < infeccion)
						{
							//cout << "\t INFECTA" << endl;
							*(matriz + i) = 3;
							++nInfectados;
							--tSanas;
						}

					}
				}
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
}

int cuentaInfectados(int *matriz, int& nPersonas, int& cnt_proc, int& mid)
{
	int enfermosRestantes = 0;
	for (int i = mid * (nPersonas / cnt_proc)*T; i < mid * (nPersonas / cnt_proc)*T + (nPersonas / cnt_proc)*T; i += T)
	{
		if (*(matriz + i) == 3)
			++enfermosRestantes;
	}
	return enfermosRestantes;
}

void imprimirEstadisticas()
{

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
	int tInfectadas, tSanas, tCuradas, tInmunes, tMuertas;	//Contadores para las estadísticas de cada tic
	int infectadasT = 0, sanasT = 0, curadasT = 0, inmunesT = 0, muertasT = 0;	//Contadores para las estadísticas finales
	double infeccion, recuperacion, infectadas;
	double local_start, local_finish, local_elapsed, elapsed;
	double tPared=0;	//t=tiempo
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
		enfermosTic = 0; tInfectadas = 0, tCuradas = 0, tSanas = 0, tMuertas = 0;
		int *subMatriz = new int[(nPersonas / cnt_proc) *T];
		//iniciar acá el tiempo 
		local_start = MPI_Wtime();
		MPI_Bcast(matriz, nPersonas, MPI_INT, 0, MPI_COMM_WORLD);	//Comparte el espacio (matriz) con todos los procesos
		MPI_Scatter(matriz, nPersonas / cnt_proc * T, MPI_INT, subMatriz, nPersonas / cnt_proc * T, MPI_INT, 0, MPI_COMM_WORLD);		//Se separa el espacio total entre los procesos para realizar el movimiento de cada una de las personas

		//------------------------MOVIMIENTO-------------------------------------
		simulacion(subMatriz, nPersonas, cnt_proc, size);
		//Se comparte el espacio luego del movimiento hecho por cada uno de los procesos
		MPI_Allgather(subMatriz, (nPersonas / cnt_proc) *T, MPI_INT, matriz, (nPersonas / cnt_proc) *T, MPI_INT, MPI_COMM_WORLD); //Vuelve a reunir la submatriz en matriz
		/*	HASTA ACÁ TODO BIEN :)	*/

		//if (mid == 0)		//Verificación del movimiento, todos los hilos tienen la misma matriz
		//{
		//	cout << "\t MATRIZ MOVIDA " <<tics<< endl << endl;
		//	imprimir(matriz, nPersonas);
		//}
		if (mid == 0) //La validación(infectar, muerte y sanación se hace con el proceso prinicipal, los resultados luego son compartidos al resto de procesos)
		{
			validar(matriz, nPersonas, cnt_proc, duracion, recuperacion, infeccion, tInfectadas, tCuradas, tMuertas, tSanas);
		}

		enfermosTic = cuentaInfectados(matriz, nPersonas, cnt_proc, mid);		//Se recorre de forma paralela el espacio para buscar la cantidad de enfermos en un rango, luego se reducen y se comparten
		MPI_Allreduce(&enfermosTic, &enfermosRestantes, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);	//Reducción para saber llevar el control de cuándo se estabiliza la infección
		
		//cortar acá el tiempo y luego acumularla en una variable global
		local_finish = MPI_Wtime();
		local_elapsed = local_finish - local_start;
		MPI_Reduce(&local_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
		
		if(mid==0)
		{	//llamr al imprimir los datos tanto pot consola como por el archivo
			infectadasT += tInfectadas;
			curadasT += tCuradas;
			muertasT += tMuertas;
			sanasT += tSanas;
			tPared += elapsed;
		}

		free(subMatriz);
		++tics;
	} while (enfermosRestantes!=0);

	MPI_Barrier(MPI_COMM_WORLD);
	//acá va la impresión final (usar las variable sque terminan con T)
	
	if (mid == 0)
	{
		cout << "\t MATRIZ FINAL " << tics << endl << endl;
		imprimir(matriz, nPersonas);
		cout << endl << endl << "Time: " << elapsed << "s" << endl;
		cout << endl << "TICS totales: " << tics << endl;
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