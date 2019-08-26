#ifndef STRIATION_H
#define STRIATION_H

#include <functional>
#include <vector>

#include "mscomplex.h"

/**
 * Representation of the result of recursively carving the Morse-Smale complex
 * along faces.
 *
 * A striation is stored as a tree structure. The root of the tree represents
 * the first carved face and the carving paths used. This first carve divides
 * the Morse-Smale complex into a top and a bottom part; the two subtrees of the
 * root represent the carve operations done in the top and bottom part.
 *
 * Every striation item in the tree has an ID. The item with a given ID can be
 * obtained using item(). The root always has ID `0`.
 */
class Striation {

	public:

		/**
		 * One item from the striation.
		 */
		class Item {

			public:

				/**
				 * Creates a new striation item.
				 *
				 * \param face The ID of the Morse-Smale cell.
				 * \param topCarvePath The carve path that passes above the
				 * carved face.
				 * \param bottomCarvePath The carve path that passes below the
				 * carved face.
				 */
				Item(int face,
				     std::vector<int> topCarvePath,
				     std::vector<int> bottomCarvePath,
				     std::vector<int> topVertices,
				     std::vector<int> bottomVertices) :
				    m_face(face),
				    m_topCarvePath(topCarvePath),
				    m_bottomCarvePath(bottomCarvePath),
				    m_topVertices(topVertices),
				    m_bottomVertices(bottomVertices) {
				}

				/**
				 * The ID of the item that represents the top part of this
				 * item, or `-1` if there is no bottom part.
				 */
				int m_topItem = -1;

				/**
				 * The ID of the Morse-Smale cell.
				 */
				int m_face;

				/**
				 * The IDs of the half-edges of the carve path above the
				 * Morse-Smale cell.
				 */
				std::vector<int> m_topCarvePath;

				/**
				 * The IDs of the half-edges of the carve path below the
				 * Morse-Smale cell.
				 */
				std::vector<int> m_bottomCarvePath;

				/**
				 * The IDs of the vertices of the face that are on the top
				 * carving path.
				 *
				 * If the face is connected to the carving path by a hair, and
				 * the face is above the carving path, all of the vertices on
				 * the hair are also included.
				 */
				std::vector<int> m_topVertices;

				/**
				 * The IDs of the vertices of the face that are on the bottom
				 * carving path.
				 *
				 * If the face is connected to the carving path by a hair, and
				 * the face is below the carving path, all of the vertices on
				 * the hair are also included.
				 */
				std::vector<int> m_bottomVertices;

				/**
				 * The ID of the item that represents the bottom part of this
				 * item, or `-1` if there is no bottom part.
				 */
				int m_bottomItem = -1;
		};

		/**
		 * Creates a new, empty striation.
		 */
		Striation();

		/**
		 * Returns the item with the given ID.
		 *
		 * \param id The ID.
		 * \return The item with ID `id`.
		 */
		Item& item(int id);

		/**
		 * Adds a new item to the striation.
		 *
		 * \param face The Morse-Smale cell to add an item for.
		 * \param topCarvePath The IDs of the half-edges of the carve path above
		 * the Morse-Smale cell.
		 * \param bottomCarvePath The IDs of the half-edges of the carve path
		 * below the Morse-Smale cell.
		 * \param topVertices The IDs of the vertices of the face that are on
		 * the top carving path.
		 * \param bottomVertices The IDs of the vertices of the face that are on
		 * the bottom carving path.
		 * \return The ID of the new item.
		 */
		int addItem(int face,
		            std::vector<int> topCarvePath,
		            std::vector<int> bottomCarvePath,
		            std::vector<int> topVertices,
		            std::vector<int> bottomVertices);

		/**
		 * Performs an action for all items in the striation, from top to
		 * bottom.
		 *
		 * \param f A function to call for every striation item.
		 */
		void forItemsInOrder(const std::function<void(Item&, int)>& f);

		/**
		 * Performs an action for all items in the striation, from top to
		 * bottom, starting from some item.
		 *
		 * \param i The ID of the root item.
		 * \param f A function to call for every striation item.
		 */
		void forItemsInOrder(int i,
		                     const std::function<void(Item&, int)>& f);

		/**
		 * Returns the number of items in this striation.
		 *
		 * \note Beware of fencepost errors: there is one striation path more
		 * than there are items.
		 *
		 * \return The item count.
		 */
		int itemCount();



	private:

		/**
		 * A list of the items in the striation.
		 */
		std::vector<Item> m_items;
};

#endif // STRIATION_H
