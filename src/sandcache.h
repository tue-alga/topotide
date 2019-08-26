#ifndef SANDCACHE_H
#define SANDCACHE_H

#include <functional>

#include "mscomplex.h"
#include "network.h"
#include "striation.h"

/**
 * Class that computes the sand function between paths in a striation. Computed
 * sand function values are cached so they do not need to be computed more than
 * once.
 */
class SandCache {

	public:

		/**
		 * The type of a sand function. It gets as inputs:
		 *
		 * * the sand cache values (like in m_sandCache);
		 *
		 * * the Morse-Smale complex;
		 *
		 * * a list of striation items, ordered from top to bottom;
		 *
		 * * the ID of the origin path;
		 *
		 * * the ID of the destination path.
		 *
		 * A sand function does not return anything; it writes its result (and
		 * possibly other sand function values than the one requested - which
		 * can often be done efficiently, which is the reason why we are using
		 * a sand cache) to the sand cache.
		 */
		typedef std::function<void (std::vector<std::vector<double>>&,
		                    MsComplex*,
		                    std::vector<Striation::Item>&,
		                    int,
		                    int)> SandFunction;

		/**
		 * Creates a sand function cache for the given striation.
		 *
		 * \param msc The underlying Morse-Smale complex.
		 * \param striation The striation to construct the cache for.
		 * \param sandFunction The SandFunction to use.
		 */
		SandCache(MsComplex* msc, Striation* striation,
		          SandFunction sandFunction);

		/**
		 * Computes the sand function between two paths, using the SandFunction
		 * assigned to this sand cache.
		 *
		 * \note The sand function is not commutative, so
		 * `sandFunction(p1, p2)` is not necessarily equal to
		 * `sandFunction(p2, p1)`.
		 *
		 * \param from The path to compute the sand function from.
		 * \param to The path to compute the sand function to.
		 * \return The resulting sand function value.
		 */
		double sandFunction(const Network::Path& from, const Network::Path& to);

		double sandFunction(int fromId, int toId);

		/**
		 * SandFunction implementation for the water level model.
		 *
		 * \param sandCache The sand cache values.
		 * \param msc The Morse-Smale complex.
		 * \param striationTopBottom A list of striation items, ordered from
		 * top to bottom.
		 * \param fromId The ID of the origin path.
		 * \param toId The ID of the destination path.
		 */
		static void waterLevelSandFunction(
		        std::vector<std::vector<double>>& sandCache,
		        MsComplex* msc,
		        std::vector<Striation::Item>& striationTopBottom,
		        int fromId,
		        int toId);

		/**
		 * SandFunction implementation for the water flow model.
		 *
		 * \param sandCache The sand cache values.
		 * \param msc The Morse-Smale complex.
		 * \param striationTopBottom A list of striation items, ordered from
		 * top to bottom.
		 * \param fromId The ID of the origin path.
		 * \param toId The ID of the destination path.
		 */
		static void waterFlowSandFunction(
		        std::vector<std::vector<double>>& sandCache,
		        MsComplex* msc,
		        std::vector<Striation::Item>& striationTopBottom,
		        int fromId,
		        int toId);

		/**
		 * SandFunction implementation for the symmetric flow model.
		 *
		 * \param sandCache The sand cache values.
		 * \param msc The Morse-Smale complex.
		 * \param striationTopBottom A list of striation items, ordered from
		 * top to bottom.
		 * \param fromId The ID of the origin path.
		 * \param toId The ID of the destination path.
		 */
		static void symmetricFlowSandFunction(
		        std::vector<std::vector<double>>& sandCache,
		        MsComplex* msc,
		        std::vector<Striation::Item>& striationTopBottom,
		        int fromId,
		        int toId);

	private:

		/**
		 * The cached values. Stored as a two-dimensional vector such that
		 * `m_sandCache[from][to]` stores the sand function from the path
		 * with ID `from` to the path with ID `to`, or `-1` if this value has
		 * not been computed yet.
		 */
		std::vector<std::vector<double>> m_sandCache;

		/**
		 * The Morse-Smale complex.
		 */
		MsComplex* m_msc;

		/**
		 * A list of striation items, ordered from top to bottom.
		 */
		std::vector<Striation::Item> m_striationTopBottom;

		/**
		 * The SandFunction to use.
		 */
		SandFunction m_sandFunction;
};

#endif // SANDCACHE_H
