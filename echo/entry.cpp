#include <memory>
#include <errno.h>

#include "log.h"

#include "args.h"
#include "client.h"

int main(int argc, char **argv)
{
	if ( application_initial_argument::instance()->load(argc, argv) < 0 ) {
		loerror("nstest") << "failed load argument for application startup.";
		return 1;
	}

	int type;
	if ( (type = application_initial_argument::instance()->get_type()) < 0 ) {
		loerror("nstest") << "unknown application run type specified.";
		return 1;
	}

	if (type == SESS_TYPE_SERVER) {
		loerror("nstest") << "nstest server did NOT support now.";
		return 1;
	}

	if (type == SESS_TYPE_CLIENT) {
		try {
			std::shared_ptr<session_client> clientptr = std::make_shared<session_client>();
			if ( clientptr->begin() < 0 ) {
				throw -ENOLINK;
			}
		} catch (...) {
			loerror("nstest") << "exception catched when building client shared-ptr.";
			return 1;
		}

		return 0;
	}

	return 1;
}
