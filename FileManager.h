/**
	SexyOS
	FileManager.h
	Przeznaczenie: Zawiera klas� FileManager oraz deklaracje metod i konstruktor�w

	@author Tomasz Kilja�czyk
	@version 30/12/18
*/

/*
 * Aby �atwiej nawigowa� po moim kodzie polecam z�o�y� wszystko
 * Skr�t: CTRL + M + A
 */

#ifndef SEXYOS_FILEMANAGER_H
#define SEXYOS_FILEMANAGER_H

#define _CRT_SECURE_NO_WARNINGS

#include <ctime>
#include <string>
#include <array>
#include <bitset>
#include <vector>
#include <unordered_map>

/*
	TODO:
	- doda� semafory (czekam na semafory)
	- doda� odczyt i zapis sekwencyjny
	- dorobi� stuff z �adowaniem do pliku wymiany (opcjonalne)
	- i z zapisywaniem z pliku wymiany do dysku (opcjonalne)
*/

//Do u�ywania przy funkcji open (nazwy m�wi� same za siebie)
#define OPEN_R_MODE  std::bitset<2>{ "10" }
#define OPEN_W_MODE  std::bitset<2>{ "01" }
#define OPEN_RW_MODE std::bitset<2>{ "11" }

//Klasa zarz�dcy przestrzeni� dyskow� i systemem plik�w
class FileManager {
private:
	//--------------------------- Aliasy ------------------------
	using u_int = unsigned int;
	using u_short_int = unsigned short int;



	//--------------- Definicje sta�ych statycznych -------------

	static const uint8_t BLOCK_SIZE = 32;	   	   //Rozmiar bloku (bajty)
	static const u_short_int DISK_CAPACITY = 1024; //Pojemno�� dysku (bajty)
	static const uint8_t BLOCK_INDEX_NUMBER = 3;   //Warto�� oznaczaj�ca d�ugo�� pola blockDirect
	static const uint8_t INODE_NUMBER_LIMIT = 32;  //Maksymalna ilo�� element�w w katalogu
	static const uint8_t MAX_FILENAME_LENGTH = 16; //Maksymalna d�ugo�� �cie�ki

	static const bool BLOCK_FREE = false;           //Warto�� oznaczaj�ca wolny blok
	static const bool BLOCK_OCCUPIED = !BLOCK_FREE; //Warto�� oznaczaj�ca zaj�ty blok

	//Maksymalny rozmiar danych
	static const u_short_int MAX_DATA_SIZE = (BLOCK_INDEX_NUMBER + BLOCK_SIZE / 2)*BLOCK_SIZE;

	//Maksymalny rozmiar pliku (wliczony blok indeksowy)
	static const u_short_int MAX_FILE_SIZE = MAX_DATA_SIZE + BLOCK_SIZE;



	//---------------- Definicje struktur i klas ----------------

	//Klasa i-w�z�a - zawiera podstawowe informacje o pliku
	struct Inode {
		//Podstawowe informacje
		uint8_t blocksOccupied = 0;  //Ilo�� zajmowanych blok�w
		u_short_int realSize = 0;    //Rzeczywisty rozmiar pliku (rozmiar danych)
		std::array<u_int, BLOCK_INDEX_NUMBER> directBlocks;	//Bezpo�rednie indeksy
		u_int singleIndirectBlocks; //Indeks bloku indeksowego, zpisywanego na dysku

		//Dodatkowe informacje
		tm creationTime = tm();		//Czas i data utworzenia
		tm modificationTime = tm(); //Czas i data ostatniej modyfikacji pliku

		Inode();

		virtual ~Inode() = default;

		void clear();
	};

	struct Disk {
		//Tablica reprezentuj�ca przestrze� dyskow� (jeden indeks - jeden bajt)
		std::array<char, DISK_CAPACITY> space;

		//----------------------- Konstruktor -----------------------
		Disk();

		//-------------------------- Metody -------------------------
		void write(const u_short_int& begin, const std::string& data);
		void write(const u_short_int& begin, const std::array<u_int, BLOCK_SIZE / 2>& data);

		const std::string read_str(const u_int& begin) const;
		const std::array<u_int, BLOCK_SIZE / 2> read_arr(const u_int& begin) const;
	} disk; //Struktura dysku
	struct FileSystem {
		u_int freeSpace{ DISK_CAPACITY }; //Zawiera informacje o ilo�ci wolnego miejsca na dysku (bajty)

		//Wektor bitowy blok�w (domy�lnie: 0 - wolny blok, 1 - zaj�ty blok)
		std::bitset<DISK_CAPACITY / BLOCK_SIZE> bitVector;

		/**
		 Tablica i-w�z��w
		 */
		std::array<Inode, INODE_NUMBER_LIMIT> inodeTable;
		//Pomocnicza tablica 'zaj�to�ci' i-w�z��w (1 - zaj�ty, 0 - wolny).
		std::bitset<INODE_NUMBER_LIMIT> inodeBitVector;
		std::unordered_map<std::string, u_int> rootDirectory;

		FileSystem();

		const u_int get_free_inode_id();

		void reset();
	} fileSystem; //System plik�w

	class FileIO {
	private:
		std::string buffer;
		u_short_int readPos = 0;
		Disk* disk;
		Inode* file;

		bool readFlag;
		bool writeFlag;

	public:
		FileIO() : disk(nullptr), file(nullptr), readFlag(false), writeFlag(false) {}
		FileIO(Disk* disk, Inode* inode, const std::bitset<2>& mode) : disk(disk), file(inode),
			readFlag(mode[1]), writeFlag(mode[0]) {}

		void buffer_update(const int8_t& blockNumber);

		std::string read(const u_short_int& byteNumber);
		std::string read_all();
		void reset_read_pos() { readPos = 0; }

		void write(const std::vector<std::string>& dataFragments, const int8_t& startIndex) const;

		const std::bitset<2> get_flags() const;
	};



	//------------------- Definicje zmiennych -------------------
	bool messages = false; //Zmienna do w��czania/wy��czania powiadomie�
	bool detailedMessages = false; //Zmienna do w��czania/wy��czania szczeg�owych powiadomie�
	//std::unordered_map<std::string, u_int> usedFiles;
	std::unordered_map<std::string, FileIO> accessedFiles;


public:
	//----------------------- Konstruktor -----------------------
	/**
		Konstruktor domy�lny. Przypisuje do obecnego katalogu katalog g��wny.
	*/
	explicit FileManager() = default;



	//-------------------- Podstawowe Metody --------------------
	/**
		Tworzy plik o podanej nazwie w obecnym katalogu.\n
		Po stworzeniu plik jest otwarty w trybie do zapisu.

		@param name Nazwa pliku.
		@return True, je�li operacja si� uda�a i false, je�li operacja nie powiod�a si�.
	*/
	bool file_create(const std::string& name);

	/**
		Zapisuje podane dane w danym pliku usuwaj�c poprzedni� zawarto��.

		@param name Nazwa pliku.
		@param data Dane do zapisu.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool file_write(const std::string& name, const std::string& data);

	/**
		Dopisuje podane dane na koniec pliku.

		@param name Nazwa pliku.
		@param data Dane do zapisu.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool file_append(const std::string& name, const std::string& data);

	/**
		Odczytuje podan� liczb� bajt�w z pliku. Po odczycie przesuwa si� wska�nik odczytu.\n
		Aby zresetowa� wska�nik odczytu nale�y ponownie otworzy� plik.

		@param name Nazwa pliku.
		@param byteNumber Ilo�� bajt�w do odczytu.
		@return Odczytane dane.
	*/
	std::string file_read(const std::string& name, const u_short_int& byteNumber);

	/**
		Odczytuje ca�e dane z pliku.

		@param name Nazwa pliku.
		@return Odczytane dane.
	*/
	const std::string file_read_all(const std::string& name);

	/**
		Usuwa plik o podanej nazwie znajduj�cy si� w obecnym katalogu.\n
		Plik jest wymazywany z katalogu g��wnego oraz wektora bitowego.

		@param name Nazwa pliku.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool file_delete(const std::string& name);

	/**
		Otwiera plik z podanym trybem dost�pu:
		- R (read) - do odczytu
		- W (write) - do zapisu
		- RW (read/write) - do odczytu i zapisu

		@param name Nazwa pliku.
		@param mode Tryb dost�pu do pliku.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool file_open(const std::string& name, const std::bitset<2>& mode);

	/**
		Zamyka plik o podanej nazwie.

		@param name Nazwa pliku.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool file_close(const std::string& name);



	//--------------------- Dodatkowe metody --------------------

	/**
		Formatuje dysk. Zeruje wektor bitowy, usuwa wszystkie i-w�z�y,
		tworzy nowy katalog g��wny.

		@return void.
	*/
	bool disk_format();

	/**
		Tworzy plik o podanej nazwie w obecnym katalogu i zapisuje w nim podane dane.
		Po stworzeniu plik jest otwarty w trybie do zapisu.

		@param name Nazwa pliku.
		@param data Dane typu string.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool file_create(const std::string& name, const std::string& data);

	/**
		Zmienia nazw� pliku o podanej nazwie.

		@param name Obecna nazwa pliku.
		@param newName Zmieniona nazwa pliku.
		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool file_rename(const std::string& name, const std::string& newName);

	/**
		Zmienia zmienn� odpowiadaj�c� za wy�wietlanie komunikat�w.
		false - komunikaty wy��czone.
		true - komunikaty w��czone.

		@param onOff Czy komunikaty maj� by� w��czone.
		@return void.
	*/
	void set_messages(const bool& onOff);

	/**
		Zmienia zmienn� odpowiadaj�c� za wy�wietlanie szczeg�owych komunikat�w.
		false - komunikaty wy��czone.
		true - komunikaty w��czone.

		@param onOff Czy komunikaty maj� by� w��czone.
		@return void.
	*/
	void set_detailed_messages(const bool& onOff);

	//------------------ Metody do wy�wietlania -----------------

	/**
		Wy�wietla parametry systemu plik�w.

		@return void.
	*/
	static void display_file_system_params();

	/**
		Wy�wietla informacje o wybranym katalogu.

		@return void.
	*/
	void display_root_directory_info();

	/**
		Wy�wietla informacje o pliku.

		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	bool display_file_info(const std::string& name);

	/**
		Wy�wietla struktur� katalog�w.

		@return True, je�li operacja si� uda�a lub false, je�li operacja nie powiod�a si�.
	*/
	void display_root_directory();

	/**
		Wy�wietla zawarto�� dysku jako znaki.
		'.' - puste pole.

		@return void.
	*/
	void display_disk_content_char();

	/**
		Wy�wietla wektor bitowy.

		@return void.
	*/
	void display_bit_vector();


private:
	//------------------- Metody Sprawdzaj�ce -------------------

	const bool check_if_name_used(const std::string& name);

	const bool check_if_enough_space(const u_int& dataSize) const;



	//-------------------- Metody Obliczaj�ce -------------------

	static const u_int calculate_needed_blocks(const size_t& dataSize);

	const size_t calculate_directory_size();

	const size_t calculate_directory_size_on_disk();



	//--------------------- Metody Alokacji ---------------------

	void file_truncate(Inode* file, const u_int& neededBlocks);

	void file_add_indexes(Inode* file, const std::vector<u_int>& blocks);

	void file_deallocate(Inode* file);

	void file_allocate_blocks(Inode* file, const std::vector<u_int>& blocks);

	void file_allocation_increase(Inode* file, const u_int& neededBlocks);

	const std::vector<u_int> find_unallocated_blocks_fragmented(u_int blockNumber);

	const std::vector<u_int> find_unallocated_blocks_best_fit(const u_int& blockNumber);

	const std::vector<u_int> find_unallocated_blocks(const u_int& blockNumber);



	//----------------------- Metody Inne -----------------------

	std::string get_file_data_block(Inode* file, const int8_t& indexNumber) const;

	void file_write(Inode* file, FileIO* IO, const std::string& data);

	void file_append(Inode* file, FileIO* IO, const std::string& data);

	static const tm get_current_time_and_date();

	void change_bit_vector_value(const u_int& block, const bool& value);

	static const std::vector<std::string> fragmentate_data(const std::string& data);
};

#endif //SEXYOS_FILEMANAGER_H
