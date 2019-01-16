﻿#include "Shell.h"
#include <iostream>
#include <string>
#include <algorithm>


//Metody pracy shella
void Shell::boot() //Funckja startująca pętlę shella
{
	tree.fork(new PCB("shell", 1));
	logo();
	//PlaySound(TEXT("Startup.wav"), NULL, SND_ALIAS);
	loop();
}

void Shell::logo() {

	std::cout << "                                                                                                                          " << std::endl;
	std::cout << "                                                                                                                          " << std::endl;
	std::cout << "                                                                  `:ossss+-       .+syys+.                                " << std::endl;
	std::cout << "                                                                .yNo.   .hMh`   .dd-  `/M/                                " << std::endl;
	std::cout << "            `:oso/     `:oso.    `/o/     -oo- -oo-      ++/   sMd.      `MMy   dMo     :                                 " << std::endl;
	std::cout << "            +N/..hh   .ym+-yMm   -hsNMs  /ddys`soyMN`    :MM:  yMN`        NMm   oMMh:                                    " << std::endl;
	std::cout << "            mMy`     /Nd` `hMs   .  -NMoho`   `  .MM/   `dMd  :MMo        .MMy    .oNMd/                                  " << std::endl;
	std::cout << "            .hMN+   :MMo+yds-        oMM+         NMy  -mMM:  oMM/        yMN.      `oNMh                                 " << std::endl;
	std::cout << "              :NM+  sMM:.`   `     .sysMN-        yMm`+dmMd   :MMo       sMd.         /MN                                 " << std::endl;
	std::cout << "          +o:.-hd.  :NMy::/os+ `yydd:  yMN+so     :MMNy:MM:    +NN/`  .+md/   .ys:. `:hh.                                 " << std::endl;
	std::cout << "         .+oso/.     .+ss+:`   -so-     /s+.       :+. hMh      `/ossso/`     ./oyyso:`                                   " << std::endl;
	std::cout << "                                                      :Mm`                                                                " << std::endl;
	std::cout << "                                                 `   :my`                                                                 " << std::endl;
	std::cout << "                                               `sNmhy+.                                                                   " << std::endl;
	std::cout << "                                                                                                                          " << std::endl;
	std::cout << "                                                                                        ..----. .. --::.                  " << std::endl;
	std::cout << "                                                                                 .--::::::////:----::/+oso:               " << std::endl;
	std::cout << "                                                                          .-::////////:::::/:/+:-:/::::/+hy:              " << std::endl;
	std::cout << "                                                                      -:////////:-..`    ``-/:.-+o+/:///+hho-             " << std::endl;
	std::cout << "                                                                   -/////////-.            -:`-:/:-:oossshhy:             " << std::endl;
	std::cout << "                                                                  -///+++/-`              ./`.//://o+/oyhdhy/             " << std::endl;
	std::cout << "                                                                   .//++o:`               //`-:://+s+:+ydhhh+             " << std::endl;
	std::cout << "                                                                    `:/++++:.            `/-``-:/+o++oyhdhhho             " << std::endl;
	std::cout << "                                                                      .://+++/-`         `:`---:/osshdhddhyho             " << std::endl;
	std::cout << "                                                                        .:///++/::-.`    --.://+oshdddddhhyhs             " << std::endl;
	std::cout << "                                                                         `.:::/+++o+//:-..--://+osoosyhdhhhhs             " << std::endl;
	std::cout << "                                                                            .-:::/+o+++/--/.-:://---//+syhhhs             " << std::endl;
	std::cout << "                                                                               .-:://:/:..-:-......-://++shhy:            " << std::endl;
	std::cout << "                                                                                 ``--/:...-..--.-/:::///+ooyy/            " << std::endl;
	std::cout << "                                                                                     -.`...-----:+o+////+ooss+-           " << std::endl;
	std::cout << "                                                                                      ``.-:::::::/oyo+/+++osso--          " << std::endl;
	std::cout << "                                                                                  `.-:::://////////+so++++ooss/--         " << std::endl;
	std::cout << "                                                                                 -://////::://///////oo+++++oos/--        " << std::endl;
	std::cout << "                                                                                `-:///////////////////oso++++oos+--       " << std::endl;
	std::cout << "                                                                                ``-://////////////////+syo+++ooos/--      " << std::endl;
	std::cout << "                                                               .::-.`             `..:::////:::/:::////+ohhs+++ooss/-.    " << std::endl;
	std::cout << "                                                             .:///////-.`          ``.---::::::::////////hhhyo++ooss/-.   " << std::endl;
	std::cout << "                                                           .:////+++++///:.`         `:..-::::/::::://////yysooooooss:-.  " << std::endl;
	std::cout << "                                                        `.://:::///+++o+++//:-`       .:.-::::::::::://++oo+:+oooossso--  " << std::endl;
	std::cout << "                                                      `.://:--://///::::://+++/:.`   `-:---:::::::://///+ooooooossssss:-  " << std::endl;
	std::cout << "                                                    `.:/:--://++++++//////::-::::/:-`.:::::///::::://+++oooosssssss+:-    " << std::endl;
	std::cout << "                                                ` -/.-:::///++++++++++++++++++////:::///oo+////////+++ooooosssss+/:-      " << std::endl;
	std::cout << "                                              `-//://////+++++ooo+++++++++++++++:/+/++/+ooooo++/++ooosssyso-.-:           " << std::endl;
	std::cout << "                                           ` -://////+++++++oo+/--://++++++++++//+/oo+++oooo++oosyssssoo++/.              " << std::endl;
	std::cout << "                                        `-://///++++++++++++/:`     `.-:/+++++++++ooossssoosso+++++/::////:               " << std::endl;
	std::cout << "                                  .-://////++++++++++++//-.`           `-/++++++++++++++++++o+:::://///// :               " << std::endl;
	std::cout << "                                `-::///////+++oooooo++//:-                .:/++oooooooo+++++ooo+//////////:`              " << std::endl;
	std::cout << "                             :-////+oo++++++ooo+/--```                      `.-:/+ooooooooooooooss+///////::`             " << std::endl;
	std::cout << "                        `:://+ooossssoo,,``                                     -:+oooosssssssssso//////:::  `            " << std::endl;
	std::cout << "                     `:/+++++o,,,`````                                               -:+oosssssssyyyys+////::``           " << std::endl;
	std::cout << "                    `:/+++`````                                                          :/ossyyyyyyyyyyyo+/.``           " << std::endl;
	std::cout << "                 ,`:/+`                                                                     ```/////////::-               " << std::endl;
	std::cout << "                                                                                                                          " << std::endl;

} //Wyswietlanie loga systemu

void Shell::loop() //Pętla shella
{
	do {
		read_line();
		execute();

		//Czyszczenie
		line.clear();
		parsed.resize(0);

	} while (status == true);

}

void Shell::read_line() //Odczyt surowych danych
{
	cout << "$ ";
	getline(cin, line);
	parse();
}

void Shell::parse() //Parsowanie
{
	transform(line.begin(), line.end(), line.begin(), ::tolower);
	parsed.resize(0);
	line = line + ' ';
	string pom;
	for (int i = 0; i < line.size(); i++) {
		if (line[i] != ' ') {
			pom += line[i];
		}
		else { //Jeśli spacja
			parsed.push_back(pom);
			pom.clear();
		}
	}
}

void Shell::execute() {

	if (parsed[0] == "go")						//Nastepny krok pracy krokowej
	{
		go();
	}

	if (parsed[0] == "help")					//Wyswietalnie listy poleceń
	{
		help();
	}

	else if (parsed[0] == "exit")				//Kończenie pracy
	{
		exit();
	}

	else if (parsed[0] == "cls")				//Kończenie pracy
	{
		cls();
	}

	else if (parsed[0] == "cp")					//Tworzenie procesu
	{
		cp();
	}

	else if (parsed[0] == "lp")					//Lista PCB wszystkich procesów
	{
		lp();
	}

	else if (parsed[0] == "lt")					//Drzewo procesow
	{
		lt();
	}

	else if (parsed[0] == "dp")					//Usuwanie procesu
	{
		dp();
	}

	else if (parsed[0] == "ls")					//Listowanie katalogu
	{
		ls();
	}

	else if (parsed[0] == "cf")					//Utworzenie pliku
	{
		cf();
	}

	else if (parsed[0] == "df")					//Usunięcie pliku
	{
        df();
	}

	else if (parsed[0] == "ld")					//Listowanie zawartości wskazanego bloku dyskowego
	{
        ld();
	}

	else if (parsed[0] == "wf")					//Zapisywanie do pliku
	{
		wf();
	}

	else if (parsed[0] == "fo")					//Otwarcie pliku
	{
		fo();
	}

	else if (parsed[0] == "finfo")					//Informacje o pliku
	{
		finfo();
	}

	else if (parsed[0] == "dinfo")					//informacje o katalogu
	{
		dinfo();
	}

	else if (parsed[0] == "dskchar")					//Wyswietlanie dysku
	{
		dskchar();
	}
    else if (parsed[0] == "fsysparam")					//Parametry systemu plików
    {
        fsysparam();
    }
	else if (parsed[0] == "bitvector")					//Parametry systemu plików
	{
		bitvector();
	}
	else if (parsed[0] == "showmem")					//Wyswietlenie pamieci
	{
		showmem();
	}

	else if (parsed[0] == "showpagefile")					//Wyswietlenie pliku stronicowania
	{
		showpagefile();
	}

	else if (parsed[0] == "showpagetable")					//Wyswietlenie page table
	{
		showpagetable();
	}

	else if (parsed[0] == "showstack")					//Wyswietlenie stosu
	{
		showstack();
	}

	else if (parsed[0] == "showframes")					//Wyswietlenie ramek
	{
		showframes();
	}

    else if (parsed[0] == "ver")					//Creditsy
    {
        ver();
    }

	else if (parsed[0] == "thanks")					//Creditsy
	{
		thanks();
	}

	else if(parsed[0]==""){
		go();
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

//Commands functions
//Metody interpretera;
void Shell::go(){
		cout << "Nastepny krok" << endl;
		if(!p.ReadyPCB.empty()){
			if(!inter.execute_line(p.ReadyPCB.front()->name)){
				PCB* tempProc = p.ReadyPCB.front();
				tree.exit(tempProc->PID);
				p.Check();
			}
		}
}
//Metody shella
void Shell::ver(){
	logo();
	std::cout << "sexySO - Version 1.01.185 \n";
	std::cout << "<c> 2018 sexySO PUT Labolatory Group. All Rights Reserved. \n\n";
	std::cout << "Created by:\n";
	std::cout << "Tomasz Kiljanczyk \t File and directory management \n";
	std::cout << "Wojciech Kasperski \t RAM management \n";
	std::cout << "Michal Kalinowski \t Interface \n";
	std::cout << "Juliusz Horowski \t Process management \n";
	std::cout << "Marcin Jasinski \t Assembler commands and interpreter \n";
	std::cout << "Norbert Mlynarski \t CPU management \n";
	std::cout << "Krzysztof Kretkowski \t Inter-process communication \n";
	std::cout << "Aleksandra Laskowska \t Synchronization mechanisms \n";
	std::cout << "Alicja Gratkowska \t Virtual memory management \n\n\n";
}
void Shell::help() //Wyświetlenie listy poleceń
{
	cout << "cp [name] [program]          Tworzenie procesu				                  " << endl;
	cout << "lp                           Lista PCB wszystkich procesów                   " << endl;
	cout << "ls                           Listowanie katalogu                             " << endl;
	cout << "cf [name]                    Utworzenie pliku                                " << endl;
	cout << "df [name]                    Usuniecie pliku                                 " << endl;
	cout << "ld [block_number]            Listowanie zawartosci wskazanego bloku dyskowego" << endl;
	cout << "exit                         Konczenie pracy                                 " << endl;
}
void Shell::exit() //Kończenie pracy
{
    ver();
	status = false;
}
void Shell::cls(){
    if (parsed.size() == 1)
    {
        system("cls");
    }
    else
        cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

//Metody zarzadzania procesami
void Shell::cp() //Tworzenie procesu
{
	if(parsed[1] == "shell" || parsed[1] == "systemd"){
		cout << "Nie można stworzyć procesu " << parsed[1] << ".\n";
		return;
	}
	 else if (parsed.size() == 3)
	{
		tree.fork(new PCB(parsed[1], 2), parsed[2], 128);
		p.AddProces(tree.find_proc(parsed[1]));
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;

}

void Shell::lp() //Lista PCB wszystkich procesów
{
	if (parsed.size() == 1) 
	{
		p.displayPCBLists();
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::lt() {
	tree.display_tree();
}

void Shell::dp() {
	if(parsed[1] == "shell" || parsed[1] == "systemd") {
		std::cout << "Odmowa dostepu!" << endl;
		return;
	}
	else {
		PCB* tempProc = tree.find_proc(parsed[1]);
		if(tempProc != nullptr) {
			tree.exit(tempProc->PID);
		}
		else {
			std::cout << "Nie znaleziono procesu!\n";
		}
	}
}
//Metody dyskowe
void Shell::ls() //Listowanie katalogu
{
	if (parsed.size() == 1) 
	{
		fm.display_root_directory();
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::cf() //Utworzenie pliku
{
	if (parsed.size() == 2)
	{
		fm.file_create(parsed[1], "shell");
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::df() //Usunięcie pliku
{
	if (parsed.size() == 2) 
	{
		fm.file_delete(parsed[1], "shell");
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::ld() //Listowanie zawartości wskazanego bloku dyskowego
{
	if (parsed.size() == 1)
	{
		fm.display_root_directory();
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::wf() {
	if (parsed.size() == 3)
	{
		fm.file_write(parsed[1], "shell", parsed[2]);
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::fo() {
	if (parsed.size() == 3)
	{
		unsigned int arg;
		if(parsed[2]=="-r") arg = FILE_OPEN_R_MODE;
		if(parsed[2] =="-w") arg = FILE_OPEN_W_MODE;
		fm.file_open(parsed[1], "shell", arg);
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::fc() {
	if (parsed.size() == 2)
	{
		fm.file_close(parsed[1], "shell");
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::finfo(){
	if (parsed.size() == 2)
	{
		fm.display_file_info(parsed[1]);
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::dinfo(){
	if (parsed.size() == 1)
	{
		fm.display_root_directory_info();
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::dskchar(){
    if (parsed.size() == 1)
    {
        fm.display_disk_content_char();
    }
    else
        cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::fsysparam(){
    if (parsed.size() == 1)
    {
        fm.display_file_system_params();
    }
    else
        cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::bitvector(){
	if (parsed.size() == 1)
	{
		fm.display_bit_vector();
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}
//Metody pamieci
void Shell::showmem() {
	if (parsed.size() == 1)
	{
		mm.showMem();
	}
	else if(parsed.size() == 3){
		int begin = stoi(parsed[1]);
		int bytes = stoi(parsed[2]);
		mm.showMem(begin, bytes);
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::showpagefile() {
	if (parsed.size() == 1)
	{
		mm.showPageFile();
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::showpagetable() {
	if (parsed.size() == 2)
	{
		mm.showPageTable(tree.find_proc(parsed[1])->pageList);
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::showstack() {
	if (parsed.size() == 1)
	{
		mm.showStack();
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::showframes() {
	if (parsed.size() == 1)
	{
		mm.showFrames();
	}
	else
		cout << "Nie rozpoznano polecenia! Wpisz \"help\" by wyswietlic pomoc" << endl;
}

void Shell::thanks() {
cout<<  "                                                                              \n"
        "   //////// 	 //////  //////      //////   //    //////   ////  ////         \n"
		" ////        //    /// //    //    //   //  //     //     //    //             \n"
        "///    //// //    /// //     //   //////   //     /////   ////  ////           \n"
		"///    /// //    /// //     //   //    // //     //         //    //           \n"
        "/////////   ///////  ///////     //////   ///// //////  ///// /////            \n"
		"                                                                               \n";
cout << "                       ,,,/////#####(///,/%%%#)                                \n"
		"                     /*....                   .#&                              \n"
  		"                  /*,...,....                  ..#&                            \n"
		"                (*,,,,...                       ..,#%&                         \n"
  		"              (**,,....             ...,,,,....       #%                       \n"
		"             (*,,....  .         ..,,,,,,.......       .#%                     \n"
		"            (*,,,............ .,*/*,,.,..........        .%%                   \n"
		"           (***,,....,..... .*/***,,,.....,.......         (%                  \n"
		"          (*,*,,,,..,,...,,////*,,,,.....,,,...,,..         ,(#                \n"
		"         (//***,,.,,,,,*/////*,,,,,,,..,,,,,,,,,,,..         *(#               \n"
		"        (///*******////((//**,,,,,,,,,,,,,,,,,,,,,,,.         *(               \n"
		"       ((///(((#%###(((//*****,****,,,********,**,,,,...      /##              \n"
		"       (((//(#%###(((///***********,,**********//*,..,,....   .##              \n"
		"      ((((///(####((((//***************************,,,,.,....  /#              \n"
		"      ((/////(#####((((//****,****,,,**,,,,,,,,,,***,,,,.,...*,,*              \n"
		"      (((**/((######(((///**,,,,*******,,,,,,,,,******,,.*,.**(##))            \n"
		"      ((/*///((#####((((((////***/////#((#(###/#/##*****,##*#///((((           \n"
		"       (//((((####((/////#(((//**((##/%/((//*****,##(#*##./(//((/#(            \n"
		"        (/((((##%#(##(#/##%###*,#/#/%%&%&&&%#***/(***#...,##((((##             \n"
		"        ((((((####((##///%##%%#/#*#(((######(#/***/**/#(/*/##/(/(((            \n"
		"          #(#(((##(((#%&&%%%%%%%(**#(/////*****,,,,/,*#////(/*///(((           \n"
		"           #((###/(##%#########%(*,*#(****,,,,,,,,*,##*(//***,,*/((/           \n"
		"            ###((/(##(////(/((##/*,,,*/#,##,#,#,###/,*////***,,*#(             \n"
		"             #(/(####((/*///(#%/*,,,,,,,*(%%#(//**,***////(***/##              \n"
		"              %#%#/(#((/////##((/****//##(((###(//////////**(##                \n"
		"                #%%###((((#%%%%%%#(((#((///*/(###(///(////(##                  \n"
		"                ###%###((##%###%%%%###(/***//(###(((((//(/##                   \n"
		"                 (##%%####%%#######((((((###%%##(((((((/(/                     \n"
		"                  %%%%####%#%&&%%%%###((/##(////(//(((/(/*                     \n"
		"                   %%%%%#((###%%&&%%#%#///******///((//(*/                     \n"
		"                    #%%#%#((((#####((((((//*******/(///(*/                     \n"
		"                      ####%#(((((((((((///********((/((///                     \n"
		"                      @@%##%##((//(////*********/(###(///*((                   \n"
		"           ##%###%%%&@@@@&%#%%#(//////*******//(##%#(/////(((%%%               \n"
		"      (###%%%##%%%%#%&&@@@&&%%%#((//////((((###%%##(//////(##%%%#(###          \n"
		"    #(##%%%%%%#%&&%#%&@&@@@&&&%%%%#####%%%%%%%%##((//(///(((%%%##%#######      \n"
		"####((#%%%%%%%%&%%#%&&&%&@@&&&&&&&&%%%%%%%%%###(///(/*/(((#%%%(#%####/(######  \n"
		"####((%%%%%%%%%&%#%&&%%&@&&&&&%%%%%%%%#######((((//*//(((#%%%&#%#////**##,,### \n"
		"###((#%%%%%%%%%%#%&&%%%&&&&&&%%%%%%%%%%%######(/////((((#%&&&%#(((/*****///####\n"
		"#####%%%%%%%%%%##%%%%%%&&%&@@&&%###(#######((////((**/(&&&%%###((//////((((( ##\n"
		"####%%%%%%%%%%##%%%%%%%%%%&&&&@&%%#(((((((//((((#%%%&&&&&&&%%##((//////(((#((##\n"
		"####%%%%%%%%%###%%%%##%%%%%%&%%@@##(((/////(&&&@@&&%%&&%%#####(((((((((((((((##\n"
		"###%%%%%%%%%##########%%%%##%%%%&&&((///(%&&@&&&&&%%%%%%######((((((((((((((###\n"
		"###%%%%%%#%###########%%%%%######%&&((%&&&&@&&&&%%%%%%%%%%###(((((/(((((/(((%##\n"
		"###%%%%#%%##############%%%########%%&&&&&&&&%%%%%%%%%#%%#####(((/////(((((((##\n"
		"###%%%#############################%(,/%%%%%%##%%%%%%########(((/////((((((((%#\n"
		"###%%########(#############((####%%%..,%#%%%%#%#############(((///(//(((((((###\n"
		"##%%%#(######(############(((((##%%%%%######################(//////(((((((((###\n"
		"##%%%##(####(############((((((#%%%%#######################((/////((((((((((##%\n"
		"##%%%##(((#(#############((((##%%%%#########(############(((///////((((((((####\n"
		"#%%%%%##((((#############((((##%%%####(####((#######(##(((((///////((((((((###%\n";

}
