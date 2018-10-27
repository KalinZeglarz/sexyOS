// Created by Wojciech Kasperski on 15-Oct-18.

#include "MemoryManager.h"

PageTable::PageTable(bool bit, int frame) : bit(bit), frame(frame) {}

MemoryManager::MemoryManager() = default;

MemoryManager::~MemoryManager() = default;

MemoryManager::PageFrame::PageFrame(std::string data) {
    while (data.size() < 16) {
        data += " ";
    }
    for (int i = 0; i < 16; i++) {
        this->data[i] = data[i];
    }
}

MemoryManager::PageFrame::PageFrame() {
    for (int i = 0; i < 16; i++) {
        this->data[i] = data[i];
    }
}

MemoryManager::ProcessFramesList::ProcessFramesList(bool isFree, int PID, int PageNumber,
                                                    std::vector<PageTable> *pageList) : isFree(isFree), PID(PID),
                                                                                        PageNumber(PageNumber),
                                                                                        pageList(pageList) {

}
