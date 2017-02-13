#pragma once
#include <atomic>
#include <cassert>
namespace xutil
{
	template<typename T, int N>
	class yqueue
	{
	public:
		inline yqueue()
		{
			begin_chunk = new chunk_t;
			assert(begin_chunk);
			begin_pos = 0;
			back_chunk = nullptr;
			back_pos = 0;
			end_chunk = begin_chunk;
			end_pos = 0;
			spare_chunk = nullptr;
		}
		inline ~yqueue()
		{
			while (true) {
				if (begin_chunk == end_chunk) {
					delete(begin_chunk);
					break;
				}
				chunk_t *o = begin_chunk;
				begin_chunk = begin_chunk->next;
				delete(o);
			}

			chunk_t *sc = spare_chunk.exchange(nullptr);
			delete (sc);
		}
		inline T &front()
		{
			return begin_chunk->values[begin_pos];
		}
		inline T &back()
		{
			return back_chunk->values[back_pos];
		}
		inline void push()
		{
			back_chunk = end_chunk;
			back_pos = end_pos;

			if (++end_pos != N)
				return;

			chunk_t *sc = spare_chunk.exchange(nullptr);
			if (sc) {
				end_chunk->next = sc;
				sc->prev = end_chunk;
			}
			else {
				end_chunk->next = new chunk_t;
				assert(end_chunk->next);
				end_chunk->next->prev = end_chunk;
			}
			end_chunk = end_chunk->next;
			end_pos = 0;
		}
		inline void pop()
		{
			if (++begin_pos == N) {
				chunk_t *o = begin_chunk;
				begin_chunk = begin_chunk->next;
				begin_chunk->prev = nullptr;
				begin_pos = 0;
				chunk_t *cs = spare_chunk.exchange(o);
				delete (cs);
			}
		}
	private:
		struct chunk_t
		{
			T values[N];
			chunk_t *prev = nullptr;
			chunk_t *next = nullptr;
		};
		chunk_t *begin_chunk;
		int begin_pos;
		chunk_t *back_chunk;
		int back_pos;
		chunk_t *end_chunk;
		int end_pos;

		std::atomic<chunk_t*> spare_chunk;

		yqueue(const yqueue&) = delete;
		const yqueue &operator = (const yqueue&) = delete;
	};
}
