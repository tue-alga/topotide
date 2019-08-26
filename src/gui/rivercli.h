#ifndef RIVERCLI_H
#define RIVERCLI_H

#include <QStringList>

/**
 * The implementation of the command-line interface.
 */
class RiverCli {

    public:

        /**
         * Runs the program: parses arguments and executes the computation based
         * on those.
         *
         * \param args The command-line arguments, for example as returned by
         * the QCoreApplication.
         * \return An exit code (0 is success).
         */
		static int runComputation(const QStringList& args);
};

#endif // RIVERCLI_H
