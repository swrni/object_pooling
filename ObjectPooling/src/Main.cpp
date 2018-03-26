#include <iostream>
#include <vector>
#include <memory>

#include "StackPool.hpp"


/*template <typename Object, size_t N=0>
class HeapBuffer {
	public:
		using BorrowPtr = std::unique_ptr<
			Object,
			BorrowPtrDeleter<HeapBuffer>
		>;

		HeapBuffer() : pool_( N ) {}
		std::size_t MaxSize() const noexcept {
			return pool_.size();
		}
		template <typename ...Args>
		BorrowPtr Create( Args&&... args ) {
			pool_.push_back(
				std::make_unique<Object>(
					std::forward<Args>( args )...
				)
			);
			Object* obj = pool_.back().get();
			return BorrowPtr( obj, *this );
		}
		void Destroy( Object* object ) {
			if ( !object ) throw;
			auto& ptr = FindObject( object );
			ptr.reset();
		}
	private:
		std::unique_ptr<Object>& FindObject( Object* object ) {
			for ( auto& ptr : pool_ ) {
				if ( ptr.get() == object ) {
					return ptr;
				}
			}
			throw;
		}
		std::vector<std::unique_ptr<Object>> pool_;
};*/


template <typename Pool>
void TestCommon() {
	constexpr const auto pool_max_size{ Pool::MaxSize() };

	Pool pool( Pool::StoreToPointer{}, Pool::InitializeStore{} );

	// Holds unique_ptrs with deleters that when destroyed, will mark those objects in pool as usable again.
	std::vector<Pool::BorrowPtr> pointer_holder;

	std::cout << "Pool initial size: " << pool.SizeLeft() << " of " << pool.MaxSize() << '\n';

	for ( auto i=0u; i<pool_max_size; ++i ) {
		if ( auto item = pool.CreateOrFail(i); ( *item % 2 == 0 ) ) {
			pointer_holder.push_back( std::move( item ) );
		}
	}

	std::cout << "Pool size left: " << pool.SizeLeft() << " of " << pool.MaxSize() << '\n';

	for ( auto i=0u; i<pointer_holder.size(); ++i ) {
		auto item = pointer_holder[i].get();
		std::cout << *item << '\n';
	}
}

void Test1() {
	using Object = int;
	using Pool = StackPool<Object, 3>;

	std::cout << "\nStackPool size: " << Pool::MaxSize() << ", Store: Object.\n";
	
	TestCommon<Pool>();
}

void Test2() {
	using Object = int;
	using Store = std::unique_ptr<Object>;
	using StoreToPointer = GetRawPointer<Object, Store>;
	using InitializeStore = Initialize<Object, Store>;
	using Traits = StackPoolTraits<Object, Store, StoreToPointer, InitializeStore>;
	using Pool = StackPool<Object, 5, Traits>;

	std::cout << "\nStackPool size: " << Pool::MaxSize() << ", Store: unique_ptr.\n";
	
	TestCommon<Pool>();
}

void Test3() {
	using Object = int;
	using Pool = StackPool<Object, 3>;

	std::cout << "\nStackPool size: " << Pool::MaxSize() << ", Store: Object.\n";
	
	Pool pool;
	std::vector<Pool::BorrowPtr> items;

	for ( auto i=0u; i<Pool::MaxSize()+1; ++i ) {
		items.push_back( pool.Create(i) );
	}
	auto crash = pool.CreateOrFail(0);

}

int main() {
	Test1();
	Test2();
	Test3();

	system( "pause" );
	return 0;
}