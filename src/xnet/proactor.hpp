#pragma once
namespace xnet
{
	class proactor : public xutil::no_copy_able
	{
	public:
		typedef timer_manager::timer_id timer_id;
		proactor()
		{
			impl = new detail::proactor_impl;
			impl->init();
		}
		proactor(proactor && _proactor)
		{
			impl = _proactor.impl;
			_proactor.impl = NULL;
		}
		~proactor()
		{
			if (impl)
			{
				impl->stop();
				delete impl;
			}
		}
		bool run()
		{
			xnet_assert(impl);
			impl->run();
			return true;
		}
		void stop()
		{
			xnet_assert(impl);
			impl->stop();
		}

		acceptor get_acceptor()
		{
			xnet_assert(impl);
			acceptor acc;
			acc.proactor_ = this;
			acc.init(impl->get_acceptor());
			return acc;
		}
		connector get_connector()
		{
			xnet_assert(impl);
			connector connector_;
			connector_.proactor_ = this;
			connector_.init(impl->get_connector());
			return std::move(connector_);
		}
		template<typename T>
		timer_id set_timer(uint32_t timeout, T &&func)
		{
			return impl->set_timer(timeout, std::forward<T>(func));
		}
		void cancel_timer(timer_id id)
		{
			impl->cancel_timer(id);
		}
		void regist_connection(connection &conn)
		{
			xnet_assert(conn.impl_);
			impl->regist_connection(*conn.impl_);
		}
	private:
		detail::proactor_impl *impl;
	};
}
