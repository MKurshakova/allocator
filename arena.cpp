#include <sys/mman.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <iostream>

struct Block {
    size_t size;
    void* beginPtr;
    bool used;
    auto getSize() -> size_t;
    auto getBeginPtr() -> void*;
    auto getUsed() -> bool;
    auto setUsed(bool) -> void;
    bool operator < (const Block& block) const {
        return (size < block.size);
    }
};

auto Block::getSize() -> size_t {
    return size;
}

auto Block::getBeginPtr() -> void* {
    return beginPtr;
}

auto Block::getUsed() -> bool {
    return used;
}

auto Block::setUsed(bool used) -> void {
    this->used = used;
}

struct Arena {
    void* arena = nullptr;
    size_t arenaSize;
    size_t arenaUsed;
    std::vector<Block> blockData{};

    auto allocate(size_t const) -> void*;
    auto deallocate(void* const) -> void;

    Arena(size_t const);
   ~Arena();
};

Arena::Arena(size_t const aSize) {
    arenaUsed = 0;
    if (aSize ==0){
        throw std::logic_error{"invalid arena size"};
    }
    arenaSize = aSize;
    arena = mmap(NULL, arenaSize, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}

Arena::~Arena(){
    munmap(arena, arenaSize);
}

auto Arena::allocate(size_t const size) -> void* {
    if(arenaUsed + size > arenaSize){
        throw std::logic_error{"not enough memory in arena"};
    }
    else if(size == 0){
        throw std::logic_error{"invalid size for allocation"};
    }
    else{
        auto blockIt = std::find_if(blockData.begin(), blockData.end(), [size](auto b){
            return b.getUsed() == false && b.getSize() >= size;
        });
        if(blockIt == blockData.end()){
            void* blockPtr = arena + arenaUsed;
            arenaUsed += size;
            const Block block{size, blockPtr, true};
            blockData.push_back(block);
            return blockPtr;
        }
        else{
            auto block = *blockIt;
            block.setUsed(true);
            arenaUsed += size;
            return block.getBeginPtr();
        }
    }
}

auto Arena::deallocate(void* const ptr) -> void{
    auto blockIt = std::find_if(blockData.begin(), blockData.end(), [ptr](auto b){
        return b.getBeginPtr() == ptr;
    });
    if(blockIt == blockData.end()){
        throw std::logic_error{"the address does not belong to this arena"};
    }
    else{
        auto block = *blockIt;
        block.setUsed(false);
        arenaUsed -= block.getSize();
        std::sort(blockData.begin(), blockData.end());
    }
}

auto main()-> int
{
    auto arena = Arena(4096);

    auto n = static_cast<int*>(arena.allocate(sizeof(int)));
    auto m = static_cast<int*>(arena.allocate(sizeof(int)));
    std::cout << n << "\n";
    std::cout << m << "\n";
    std::cout << *n << "\n";
    std::cout << *m << "\n";
    *n = 42;
    std::cout << *n << "\n";
    arena.deallocate(n);

    auto x = static_cast<int*>(arena.allocate(sizeof(int)));
    std::cout << (x == n) << "\n";
    std::cout << *x << "\n";

return 0;
}
