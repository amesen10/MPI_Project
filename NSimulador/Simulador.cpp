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

	//Se llena el vector con los enfermos que se tienen como párametro nInicialInfectados
	for (int iter = 0; iter < nInicialInfectados * T; iter += T)
	{
		*(matriz + iter) = 3;							//Estado 3 (Infectado)
		*(matriz + iter + 1) = 0;						//Dias infectados
		*(matriz + iter + 2) = distributionXY(generator);	//Posición en Eje-X
		*(matriz + iter + 3) = distributionXY(generator);	//Posición en Eje-Y
	}

	//Llenado del vector con la demás cantidad de personas que no están infectadas, se hace un random que decide si es inmune o sano
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

//Método que imprime el vector
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

//Método que se encarga de realizar el movimiento de cada una de las personas
void simulacion(int *subMatriz, int& nPersonas, int& cnt_proc, int& size)
{
	default_random_engine generator;
	uniform_int_distribution <int> distributionXY(0, 1);
	for (int iter = 0; iter < (nPersonas / cnt_proc)*T; iter += T)
	{						
		if (*(subMatriz + iter) == 3) //Estado 3 (Infectado)
			subMatriz[iter + 1] = subMatriz[iter + 1] + 1;	//Dias infectados

		//Si la persona no está muerta (Estado 0) se desplaza por el espacio 
		if (*(subMatriz + iter) != 0)
		{
			if (!distributionXY(generator))	//se moverá un espacio a la derecha
			{
				*(subMatriz + iter + 2) = (*(subMatriz + iter + 2) - 1); //Movimiento en Eje-X
				if (*(subMatriz + iter + 2) < 0)
					*(subMatriz + iter + 2) = size - 1;
			}
			else   //Movimiento hacia la izquierda
				*(subMatriz + iter + 2) = (*(subMatriz + iter + 2) + 1) % size;

			if (!distributionXY(generator))	//se moverá un espacio hacia abajo
			{
				*(subMatriz + iter + 3) = (*(subMatriz + iter + 3) - 1); //Movimiento en Eje-Y
				if (*(subMatriz + iter + 3) < 0)
					*(subMatriz + iter + 3) = size - 1;
			}
			else	//se moverá un espacio hacia arriba
				*(subMatriz + iter + 3) = (*(subMatriz + iter + 3) + 1) % size;
		}
	}
}

//Método para realizar el contagio de las personas, así como evaluar si después de X duración, la persona se cura o se muere
//Recibe solo una porción del espacio de tamaño nPersonas/nProcesos*T
//mInfectado es una matriz que contiene en cada una de sus celdas la cantidad de enfermos que hay en dicho posición
void validar(int *matriz, int **mInfectados, int& nPersonas, int& cnt_proc, int& duracion, double& recuperacion, double& infeccion, int& nInfectados, int& tCuradas, int& tMuertas, int& tSanas)
{
	default_random_engine generator;
	uniform_real_distribution <double> proba(0, 1);

	int  posicionEnfermos = 1, contador = 0;
	for (contador; contador < (nPersonas / cnt_proc)*T; contador += T)		//Recorre cada una de las personas del subarreglo
	{
		if (*(matriz + contador) == 2)	//Evalua si la Persona actual está Sana par aevaluar si se puede infectar dado que comparte celda con in infectado
		{
			if (mInfectados[*(matriz + contador + 2)][*(matriz + contador + 3)] != 0)	//Si esta persona sana comparte celda con algún enfermo se calcula la probabilidad de infectarlo
			{
				//Se hace el cálculo de la probabilidad y se asigna el nuevo estado en caso de darse el contagio
				if (proba(generator)*mInfectados[*(matriz + contador + 2)][*(matriz + contador + 3)] < infeccion)
				{
					//cout << "\t INFECTA" << endl;
					*(matriz + contador) = 3;		//Actualiza ele estado de esta persona a "Infectado"
					++nInfectados;
				}
			}
		}
		else
			if (*(matriz + contador) == 3)
			{
				//	2-Verifica si ya es tiempo de morir o de recuperarse
				if (*(matriz + contador + 1) == duracion)
				{
					if (proba(generator) < recuperacion)//Calcula probabilidad de recuperacion o de muerte de acuerdo a recuperacion
					{
						*(matriz + contador) = 0;		//Se actualiza el estado a Muerto=0
						//cout << "\t MUERE" << endl;	
						++tMuertas;						//Incrementa la cantidad de muertos en cada tic (tMuertas)
					}
					else
					{
						*(matriz + contador) = 1;	//Se convierte a Inmune i.e. se Cura
						//cout << "\t CURA" << endl;	
						++tCuradas;					//icrementa la cantidad de personas curadas en cada tic
					}
				}
			}
	}
}

//Método para rellenar la matriz infectados, en donde el valor de cada celda representa la cantidad de personas infectadas que hay en dicha posición
void llenarMatrizEnfermos(int *matriz, int** infectados, int& nPersonas)
{
	for (int i = 0; i < nPersonas *T; i += T)
	{
		if (*(matriz + i) == 3)		//Si la persona actual está infectada, se usan su coordenadas en la matriz y se incrementa la cantidad personas infectadas en esa celda en uno
		{
			infectados[*(matriz + i + 2)][*(matriz + i + 3)] = infectados[*(matriz + i + 2)][*(matriz + i + 3)] + 1;
		}
	}
}

//Función que retorna la cantidad de personas en enfermas a las que tiene acceso un proceso
int cuentaInfectados(int *matriz, int& nPersonas, int& cnt_proc, int& mid, int& tInfectadas)
{
	int enfermosRestantes = 0;
	for (int i = 0; i < (nPersonas / cnt_proc)*T; i += T)	//Recorre el subarreglo buscando personas enfermas
	{
		if (*(matriz + i) == 3)
			++enfermosRestantes;
	}
	return enfermosRestantes;
}

//Función que retorna la cantidad de personas que se encuentran sanas; recibe como parámetro un subarreglo
int cuentaSanos(int *matriz, int& nPersonas, int& cnt_proc, int& mid)
{
	int sanosRestantes = 0;
	for (int i = 0; i < (nPersonas / cnt_proc)*T; i += T)	//Recorre el subarreglo buscando personas sanas para incrementar dicha cantidad
	{
		if (*(matriz + i) ==2)
		{
			++sanosRestantes;
		}
	}
	return sanosRestantes;
}

//Procedimiento que se encarga de imprimir, tanto en consola como en un archivo .txt, los datos recibidos como parámetro correspondientes a la actividad de cada tic
void imprimirEstadisticas(int& nPersonas,int& tInfectadas,int& enfermosRestantes, int&  tSanas, int&  tInmunes, int&  tMuertas, int&  tics)
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

//Procedimiento que recibe los parámetros que resumen toda la simulación; estos datos se despliegan en consola y en el archivo .txt de salida
void EstadisticasFinales(int& nPersonas, int& const infectadasT, int& tSanas, int& sanasI, int&  curadasT, int&  inmunesT, int&  muertasT, int&  tics)
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

	int nPersonas, duracion, size, tics = 0;									//Variables que tomarán los valores recibidos como argumentos
	int tInfectadas=0, tSanas = 0, tCuradas = 0, tInmunes = 0, tMuertas = 0;	//Contadores para las estadísticas de cada tic
	int infectadasT = 0, sanasT = 0, curadasT = 0, inmunesT = 0, muertasT = 0;	//Contadores para las estadísticas finales
	double infeccion, recuperacion, infectadas;									//Variables que tomarán los valores recibidos como argumentos
	double local_start, local_finish, local_elapsed, elapsed, tPared = 0;		//Variables para llevar un registro del tiempo de ejecución de la simulación
	int inf=0, cur=0, mue=0, san=0;												//Variables temporales para reducir los contadores de cada proceso para las estadísticas
	ofstream archivo;															//Archivo para la impresión de los datos de salida 

	obt_args(argv, nPersonas, infeccion, recuperacion, duracion, infectadas, size);	//Invocación al método que obtiene los datos de los argumentos

	int *matriz = new int[nPersonas * 4];	//puntero a un arreglo que simula dos dimensiones por donde se van a desplazar las personas
	
	int nInicialInfectados = nPersonas * infectadas;	//Contiene el número de personas que deben estar contagiadas antes de iniciar con la simulación

	if (mid == 0)		//Creación de la matriz en el proceso principal
	{
		archivo.open("Estadisticas.txt");	//Creación del archivo de texto
		archivo.close();
		//cout << "\t MATRIZ INICIAL" << endl << endl;
		iniciar(matriz, nPersonas, nInicialInfectados, sanasT, inmunesT, size);	//Comienza la simulación
		//imprimir(matriz, nPersonas);		//Imprime la matriz inicial
	}

	int enfermosRestantes = 0, enfermosTic = 0;			//Variables para el control de la ejecución de la simulación
	do
	{
		tInfectadas = 0, tCuradas = 0, tSanas = 0, tMuertas = 0;
		int *subMatriz = new int[(nPersonas / cnt_proc) *T];
		int **mInfectados = new int*[size];
		for (int i = 0; i < size; ++i)		//Creación del matriz que contiene la cantidad de infectados por celda
		{
			mInfectados[i] = new int[size] {0};		//Se hace blanqueamiento de los contadores
		}		

		local_start = MPI_Wtime();				//Inicio del tiempo de ejecución
		MPI_Bcast(matriz, nPersonas, MPI_INT, 0, MPI_COMM_WORLD);	//Comparte el espacio (matriz) con todos los procesos

		//Se separa el espacio total entre los procesos para realizar de forma paralela la simulación
		MPI_Scatter(matriz, nPersonas / cnt_proc * T, MPI_INT, subMatriz, nPersonas / cnt_proc * T, MPI_INT, 0, MPI_COMM_WORLD);		
		
		llenarMatrizEnfermos(matriz, mInfectados, nPersonas);		//Se invoca al método que llena la matriz de enfermos
		
		//Invocación al método que se encarga de realizar los contagios, curar y establecer la muerte de cada persona 
		validar(subMatriz, mInfectados, nPersonas, cnt_proc, duracion, recuperacion, infeccion, tInfectadas, tCuradas, tMuertas, tSanas);
		
		simulacion(subMatriz, nPersonas, cnt_proc, size);		//Se da el movimiento de cada persona por el espacio

		enfermosTic = cuentaInfectados(subMatriz, nPersonas, cnt_proc, mid, tInfectadas);	//Se obtienen la cantidad de personas infectadas a las que tiene acceso cada proceso 
		tSanas = cuentaSanos(subMatriz, nPersonas, cnt_proc, mid);							//Se obtienen la cantidad de personas sanas a las que tiene acceso cada proceso 

		//Incremento de los acumuladores para las estadíticas finales; se suman los datos producidos en cada tic y finalmente se reducen para conocer las cantidades totales
		infectadasT += tInfectadas;
		curadasT += tCuradas;
		muertasT += tMuertas;

		MPI_Allreduce(&enfermosTic, &enfermosRestantes, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);	//Reducción para saber llevar el control de cuándo se estabiliza la infección
		
		//Se reducen los datos producidos por cada proceso en un tic, para usarlas en la impresión de las estadísticas de cada tic
		MPI_Reduce(&tSanas, &san, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
		MPI_Reduce(&tInfectadas, &inf, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
		MPI_Reduce(&tCuradas, &cur, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
		MPI_Reduce(&tMuertas, &mue, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

		//Vuelve a reunir la submatriz en la matriz luego de que cada proceso se encargara de realizar las operaciones 
		//correspondientes en el proceso 0, para que este la vuelva a compartir en el Bcast al inicio del ciclo
		MPI_Gather(subMatriz, (nPersonas / cnt_proc) *T, MPI_INT, matriz, (nPersonas / cnt_proc) *T, MPI_INT, 0, MPI_COMM_WORLD); 
		
		//Se captura la duración de la ejecución del tic
		local_finish = MPI_Wtime();
		local_elapsed = local_finish - local_start;
		MPI_Reduce(&local_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
		
		
		if (mid == 0)	//Se imprimen las estadísticas correspondientes a cada tic
		{
			tPared += elapsed;		//Acumula el timpo de ejecución de cada Tic
			imprimirEstadisticas(nPersonas, inf, enfermosRestantes, san, cur, mue, tics);	//Imprime las estadísticas correspondientes a cada tic
		}

		//Se libera la memoria utilizada por el subarrelgo utilizado por cada proceso
		free(subMatriz);
		for (int i = 0; i < size; ++i)		//Se libera la memoria utilizada por la matriz de personas infectadas
			delete[] mInfectados[i];
		delete[] mInfectados;
		++tics;		//Se incrementa la cantidad de tics 
	} while (enfermosRestantes!=0);		//Todo lo anterior se va a ejecutar al menos una vez y tantas mientras hayan personas infectadas en el espacio
	inf = 0, cur = 0, mue = 0;			//Se reestablecen las variables para las reducciones de los datos para las estadísticas finales
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Reduce(&infectadasT, &inf, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(&curadasT, &cur, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(&muertasT, &mue, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	if (mid == 0)
	{
		inf += nInicialInfectados;
		EstadisticasFinales(nPersonas, inf, san/*para los sanos restantes*/, sanasT, cur, inmunesT, mue, tics);		//Se imprimen las estadísticas finales, se envían como parámetros las reducciones realizadas

		//cout << "\t MATRIZ FINAL " << tics << endl << endl;
		//imprimir(matriz, nPersonas);	//Impresión de la matriz luego de la simulación

		//Impresión de los datos finales referentes al tiempo total de ruación y los tics
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
	MPI_Finalize();		//Finalización de la ejecución paralela
	return 0;
}

void uso(string nombre_prog) {
	cerr << nombre_prog.c_str() << " secuencia de parametros de entrada " << endl;
	exit(0);
}
//Método encargado de guardar en los parámetros recibidos, los datos que vienen como argumentos
void obt_args(char* argv[], int& numeroPersonas, double& infeccion, double& recuperacion, int& duracion, double& infectadas, int& size) {

	numeroPersonas = strtol(argv[1], NULL, 10);
	infeccion = stod(argv[2]);
	recuperacion = stod(argv[3]);
	duracion = strtol(argv[4], NULL, 10);
	infectadas = stod(argv[5]);
	size = strtol(argv[6], NULL, 10);

	//Validaciones en caso que algún dato ingresado como argumento sea erróneo
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
	case 1: size = /*6*/100; break;
	case 2: size = 500; break;
	case 3: size = 1000; break;
	}
}