#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <memory>

#include <numeric> // std::iota.

struct EmptyType {};

template <typename Object, typename Store = EmptyType>
struct Initialize {
	static_assert( !std::is_same<Object,Store>(), "Object and Store must differ" );
	// Simple initializer.
	template <typename ...Args>
	void operator()( Object& object, Args&&... args ) const {
		object = Object(
			std::forward<Args>( args )...
		);
	}
	// std::unique_ptr initializer.
	template <typename ...Args>
	void operator()( Store& store, Args&&... args ) const {
		store = std::make_unique<Object>(
			std::forward<Args>( args )...
		);
	}
};

template <typename Object, typename Store = EmptyType>
struct GetRawPointer {
	Object* operator()( Object& object ) const {
		return &object;
	}
	Object* operator()( Store& store ) const {
		return store.get();
	}
};

template <	typename Object,
			typename _StoreObject = Object,
			typename _StoreToPointer = GetRawPointer<Object>,
			typename _InitializeStore = Initialize<Object>>
struct StackPoolTraits {
	using StoreObject = _StoreObject;
	using StoreToPointer = _StoreToPointer;
	using InitializeStore = _InitializeStore;
};


template <typename Pool>
class BorrowPtrDeleter {
	public:
		BorrowPtrDeleter( const BorrowPtrDeleter& ) = delete;
		BorrowPtrDeleter& operator=( const BorrowPtrDeleter& ) = delete;
		BorrowPtrDeleter& operator=( BorrowPtrDeleter&& ) = delete;

		BorrowPtrDeleter( Pool& pool ) : pool_( pool ) {}

		template <typename Object>
		void operator()( Object* object ) {
			// std::cout << "BorrowPtrDeleter::operator()\n";
			if ( !object ) {
				#ifdef _DEBUG
				std::cout << "object is nullptr\n";
				#endif
				return;
			}
			pool_.Destroy( object );
		}
	private:
		Pool& pool_;
};

template <typename _Object, size_t N, typename Traits = StackPoolTraits<_Object>>
class StackPool {
		static_assert( N != 0, "N must be larger than zero." );
	public:
		using Object = _Object;
		using StoreObject = typename Traits::StoreObject;
		using StoreToPointer = typename Traits::StoreToPointer;
		using InitializeStore = typename Traits::InitializeStore;

		using BorrowPtr = std::unique_ptr<
			Object,
			BorrowPtrDeleter<StackPool>
		>;

		StackPool( const StackPool& ) = delete;
		StackPool& operator=( const StackPool& ) = delete;
		StackPool& operator=( StackPool&& ) = delete;
		
		template <typename StoreToPointer, typename InitializeStore>
		explicit StackPool(	StoreToPointer&& store_to_pointer,
							InitializeStore&& initialize_store )
		:	store_to_pointer_( std::forward<StoreToPointer>( store_to_pointer ) ),
			initialize_store_( std::forward<InitializeStore>( initialize_store ) ),
			unused_indices_( N ),
			used_indices_( 0 )
		{
			InitializeIndices();
		}

		StackPool()
		:	store_to_pointer_( Traits::StoreToPointer{} ),
			initialize_store_( Traits::InitializeStore{} ),
			unused_indices_( N ),
			used_indices_( 0 )
		{
			InitializeIndices();
		}

		constexpr static std::size_t MaxSize() noexcept {
			return N;
		}

		std::size_t SizeLeft() const {
			return unused_indices_.size();
		}

		template <typename ...Args>
		BorrowPtr Create( Args&&... args ) {
			// Are all objects already in use?
			if ( SizeLeft() == 0 ) {
				return CreateBorrowPtr( nullptr );
			}

			StoreObject& store = TakeUnusedStoreObject();
			initialize_store_( store, std::forward<Args>( args )... );
			return CreateBorrowPtr( store );
		}

		template <typename ...Args>
		BorrowPtr CreateOrFail( Args&&... args ) {
			if ( auto ptr = Create( std::forward<Args>( args )... ); ptr ) {
				return ptr;
			}
			throw std::bad_alloc();
		}

		void Destroy( Object* object ) {
			// TODO:: Fix this raw loop monster.
			for ( auto i=0u; i<N; ++i ) {
				if ( store_to_pointer_( buffer_[i] ) == object ) {
					unused_indices_.push_back( i );
					for ( auto j=0u; j<N; ++j ) {
						if ( used_indices_[j] == i ) {
							// Remove i (at position j) from used_indices_.
							used_indices_[j] = used_indices_.back();
							used_indices_.pop_back();
							return;
						}
					}
				}
			}
		}

	private:
		void InitializeIndices() {
			std::iota(
				std::begin( unused_indices_ ),
				std::end( unused_indices_ ),
				0
			);
			used_indices_.reserve( N );
		}

		BorrowPtr CreateBorrowPtr( Object* object ) {
			return { object, *this };
		}

		BorrowPtr CreateBorrowPtr( StoreObject& store ) {
			return { store_to_pointer_( store ), *this };
		}

		StoreObject& TakeUnusedStoreObject() {
			const std::size_t index = unused_indices_.back();
			used_indices_.push_back( index );
			unused_indices_.pop_back();
			return buffer_[ index ];
		}

	private:
		const StoreToPointer store_to_pointer_;
		const InitializeStore initialize_store_;

		std::array<StoreObject, N> buffer_;

		std::vector<std::size_t> unused_indices_;
		std::vector<std::size_t> used_indices_;

		/*const std::size_t max_object_count_;
		const std::size_t buffer_size_{ max_object_count_ * sizeof(Object) };
		std::uint8_t pool_[ buffer_size_ ];*/
};
