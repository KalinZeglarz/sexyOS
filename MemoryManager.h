// Created by Wojciech Kasperski on 15-Oct-18.
#ifndef SEXYOS_MEMORYMANGER_H
#define SEXYOS_MEMORYMANGER_H
#include <list>
#include <string>
#include <vector>
#include <map>
#include <memory>

class PCB;

//------------- Tablica stronic procesu-------------
//Indeks stronic dla każdego procesu, vector tej struktury znajduje się w PCB
struct PageTableData {
	bool bit;  //Wartość bool'owska sprawdzająca zajętość tablicy w pamięci [Sprawdza, czy ramka znajduje się w pamięci RAM]
	int frame; //Numer ramki w której znajduje się stronica

	PageTableData();
	PageTableData(bool bit, int frame);
};

class MemoryManager {

	//------------- Struktury używane przez MemoryManager'a oraz zmienne--------------
public:
	char RAM[256]; //Pamięć Fizyczna Komputera [256 bajtów]
private:
	//------------- Struktura Pojedynczej Stronicy w Pamięci -------------
	struct Page {
		char data[16]{' '}; //Dane stronicy

		Page();
		explicit Page(std::string data);

		void print() const;
	};

	//------------- Lista Ramek -------------
	//Struktura wykorzystywana do lepszego przeszukiwania pamięci ram i łatwiejszej wymiany stronic
	struct FrameData {
		bool isFree; //Czy ramka jest wolna (True == wolna, False == zajęta)
		int PID; //Numer Procesu
		int pageID; //Numer stronicy
		std::shared_ptr<std::vector<PageTableData>> pageList; //Wskaźnik do tablicy stronic procesu, która znajduje się w PCB

		FrameData(bool isFree, int PID, int pageID, std::vector<PageTableData> *pageList);
	};

	//------------- Ramki załadowane w Pamięci Fizycznej [w pamięci RAM]-------------
	std::vector<FrameData> Frames;

	//------------- Plik stronicowania -------------
	// map < PID procesu, Stronice danego procesu>
	std::map<int, std::vector<Page>> PageFile;
	//std::vector<std::pair<int, std::vector<Page>>> PageFile;

	//------------- Stos ostatnio używanych ramek (Least Recently Used Stack) -------------
	//Stos dzięki, którem wiemy, która ramka jest najdłużej w pamięci i którą ramkę możemy zastąpić
	//Jako, że mamy 256B pamięci ram, a jedna ramka posiada 16B, to będziemy mieć łącznie 16 ramek [0-15]
	//Więcej: https://pl.wikipedia.org/wiki/Least_Recently_Used
	std::list<int> Stack{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };


	//------------- Funkcje do wyświetlania bieżących stanów pamięci oraz pracy krokowej  --------------
public:
	//Pokazuje zawartość pamięci operacyjnej [RAM][fizycznej]
	void show_memory();

	//Pokazuje odpowiednie fragmenty pamięci [RAM]
	/* begin - miejsce w pamięci od którego ma być wyświetlona zawartość
	 * bytes - ilość bitów do wyświetlenia
	 */
	void show_memory(int begin, int bytes);

	//Pokazuje zawartość pliku stronicowania
	void show_page_file() const;

	//Pokazuje zawartość tablicy wymiany processu
	/* pageList - wskaźnik na tablicę stronic procesu
	 */
	static void show_page_table(const std::shared_ptr<std::vector<PageTableData>>& pageList);

	//Pokazuje Stos ostatnio używanych ramek
	void show_stack();

	//Pokazuje listę ramek w pamięci wraz z informacją do kogo dana ramka należy
	void show_frames();

	//------------- Funkcje użytkowe MemoryManagera  --------------

			//Tworzy proces bezczynności systemu umieszczany w pamięci RAM przy starcie systemu
	void memory_init();

	//Metoda ładująca program do pliku wymiany
	/* path - ścieżka do programu na dysku twardym
	 * PID - ID procesu
	 */
	int load_program(const std::string& path, int PID);

	//Usuwa z pamięci dane wybranego procesu
	void kill(int PID);

	//Tworzy wskaźnik do tablicy stronic danego procesu - funkcja wywoływana przy tworzeniu procesu
	/*  mem - potrzebna ilość pamięci
	 *  PID - ID procesu
	 */
	std::shared_ptr<std::vector<PageTableData>> create_page_list(int mem, int PID);

	//Zmienia rozmiar tablicy stronic (ma tylko zwiększać)
	void resize_page_list(int size, PCB* proc);

	//Pobiera bajt z danego adresu
	/* std::shared_ptr<PCB> process - wskaźnik do PCB danego procesu
	* int address - adres logiczny z którego chcemy pobrać bajt
	*/
	std::string get_byte(const std::shared_ptr<PCB>& process, int address);

	//Zapisuje dany fragment do pamięci w pliku wymiany
	/* *process - wskaźnik do PCB danego procesu
	 * address - adres logiczny w pamięci na którym chemy coś zapisać
	 * data - dane do zapisania w pamięci - np. dane z rejestru
	 */
	int write(const std::shared_ptr<PCB>& process, int address, std::string data);

	//Zapisuje dany fragment do pamięci w pamięci RAM bezpośrednio
	/* *process - wskaźnik do PCB danego procesu
	 * address - adres logiczny w pamięci na którym chemy coś zapisać
	 * data - dane do zapisania w pamięci - np. dane z rejestru
	 */
	int write_direct(int address, std::string data);



private:
	//Zwraca adres pierwszej wolnej ramki w pamięci
	int seek_free_frame();

	//Przesuwa ramkę podaną jako argument na początek stack'u ostatnio używanych (Least Recently Used - frames)
	/* frameID - numer ramki, którą chcemy przesunąć na początek stack'u
	 */
	void stack_update(int frameID);

	//Ładuje daną stronicę do pamięci RAM
	/*  page - stronica do załadowania
	 *  pageID - numer stronicy
	 *  PID - numer procesu
	 *  *pageList - wskaźnik na tablicę stronic procesu
	 */
	int load_to_memory(Page page, int pageID, int PID, const std::shared_ptr<std::vector<PageTableData>>& pageList);

	//Zamienia stronice zgodnie z algorytmem  podanym dla pamięci virtualnej
	/*  *pageList - wskaźnik na indeks stronic procesu
	 *  pageID - numer stronicy do zamiany
	 *  PID - ID procesu
	 * @return int zwraca numer podmienionej ramki, do której została wstawiona stronica
	*/
	int insert_page(int pageID, int PID);

public:
	//------------- Konstruktor  -------------
	MemoryManager();
	//------------- Destruktor  --------------
	~MemoryManager();
};

extern MemoryManager mm;

#endif //SEXYOS_MEMORYMANAGER_H