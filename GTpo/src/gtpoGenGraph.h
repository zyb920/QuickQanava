/*
    The MIT License (MIT)

    Copyright (c) 2016 Benoit AUTHEMAN

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

//-----------------------------------------------------------------------------
// This file is a part of the GTpo software library.
//
// \file	gtpoGenGraph.h
// \author	benoit@qanava.org
// \date	2016 01 22
//-----------------------------------------------------------------------------

#ifndef gtpoGenGraph_h
#define gtpoGenGraph_h

// STD headers
#include <list>
#include <unordered_set>
#include <memory>           // std::shared_ptr std::weak_ptr and std::make_shared
#include <functional>       // std::hash
#include <cassert>
#include <iterator>         // std::back_inserter

// GTpo headers
#include "./gtpoUtils.h"
#include "./gtpoBehaviour.h"
#include "./gtpoGenEdge.h"
#include "./gtpoGenGroup.h"
#include "./gtpoGenNode.h"
#include "./gtpoGraphConfig.h"

/*! \brief Main GTpo namespace (\#include \<GTpo\>).
 */
namespace gtpo { // ::gtpo

template <class Config>
class GenGraph;

/*! \brief Weighted directed graph using a node-list, edge-list representation.
 *
 * \image html graph-datamodel.png
 *
 * \param BaseClass Optional base class for this graph class
 *
 * \note See http://en.cppreference.com/w/cpp/language/dependent_name for
 *       typename X::template T c++11 syntax and using Nodes = typename Config::template NodeContainer< Node* >;
 *
 */
template < class Config = GraphConfig >
class GenGraph : public Config::GraphBase,
                 public gtpo::Behaviourable< gtpo::GraphBehaviour< Config >, typename Config::GraphBehaviours >
{
    /*! \name Graph Management *///--------------------------------------------
    //@{
public:
    using Configuration     = Config;
    using Graph             = GenGraph<Config>;

    using Node              = typename Config::FinalNode;
    using SharedNode        = std::shared_ptr< typename Config::FinalNode >;
    using SharedEdge        = std::shared_ptr< typename Config::FinalEdge >;
    using WeakNode          = std::weak_ptr< typename Config::FinalNode >;
    using SharedNodes       = typename Config::template NodeContainer< SharedNode >;
    using WeakNodes         = typename Config::template NodeContainer< WeakNode >;
    using WeakNodesSearch   = typename Config::template SearchContainer< WeakNode >;

    using WeakEdge          = std::weak_ptr< typename Config::FinalEdge >;
    using WeakEdges         = typename Config::template EdgeContainer< WeakEdge >;
    using SharedEdges       = typename Config::template EdgeContainer< SharedEdge >;
    using WeakEdgesSearch   = typename Config::template SearchContainer< WeakEdge >;

    using SharedGroup   = std::shared_ptr< typename Config::FinalGroup >;
    using WeakGroup     = std::weak_ptr< typename Config::FinalGroup >;
    using SharedGroups  = typename Config::template NodeContainer< SharedGroup >;

    //! User friendly shortcut to this concrete graph behaviour.
    using Behaviour = GraphBehaviour< Config >;
    //! User friendly shortcut type to this concrete graph Behaviourable base type.
    using BehaviourableBase = gtpo::Behaviourable< gtpo::GraphBehaviour< Config >,
                                                   typename Config::GraphBehaviours >;

public:
    using Size  = typename SharedNodes::size_type;

    GenGraph() noexcept :
        Config::GraphBase{},
        BehaviourableBase{} { }

    template < class B >
    explicit GenGraph( B* parent ) noexcept :
        Config::GraphBase{parent},
        BehaviourableBase{}{ }

    ~GenGraph();

    GenGraph( const GenGraph& ) = delete;
    GenGraph& operator=( const GenGraph& ) = delete;

    /*! \brief Clear the graph from all its content (nodes, edges, groups, behaviours).
     *
     * \note Graph behaviours are cleared after the topology, if you do not want to take into account topology
     * changes when clearing the graph, disable all behaviours before calling clear().
     */
    void    clear() noexcept;
public:
    /*! \brief Any already inserted node could be added as a control node to prevent it destruction when clear() is called.
     *
     * Add a node as a control node when it is used as an utility in the graph and has no meaning in topology, usually
     * a control node serializable property is set to false.
     */
    inline void         addControlNode( SharedNode node ) {
        Config::template container_adapter< SharedNodes >::insert( node, _controlNodes );
    }
    inline void         removeControlNode( SharedNode node ) {
        Config::template container_adapter< SharedNodes >::insert( node, _controlNodes );
    }
public:
    inline const SharedNodes&  getControlNodes() const noexcept { return _controlNodes; }
protected:
    SharedNodes                 _controlNodes;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Graph Node Management *///---------------------------------------
    //@{
public:
    /*! \brief Create node and insert it in the graph an return a reference to it.
     *
     * \code
     * try{
     *   gtpo::GenGraph<> g;
     *   std::weak_ptr<gtpo::GenNode<>> n1 = g.createNode();
     *   // or preferably
     *   auto n2 = g.createNode();
     * } catch ( const std::runtime_error& e ) { }
     * \endcode
     *
     * With STpo:
     * \code
     * using namespace gtpo;
     * try{
     *   gtpo::GenGraph<> g;
     *   std::weak_ptr<gtpo::Node<>> n1 = g.createNode();
     *   // or preferably
     *   auto n2 = g.createNode();
     * } catch ( const std::runtime_error& e ) { }
     * \endcode
     * \return a reference to the created node (graph has ownership for the node).
     * \throw gtpo::bad_topology_error with an error description if creation fails.
     */
    auto    createNode( ) noexcept( false ) -> WeakNode;

    /*! \brief Create a node with the given \c className and insert it into the graph.
     *
     * Default implementation only create node and edges corresponding to gtpo::GenNode::getClassName().
     * This method is mainly usefull when a hierarchy of virtual node should be serialized.
     * Always prefere createNode() and insertNode( SharedNode ) to add content in the graph until you
     * are writing custom in/out serialization functors for a serializer dealing with specialized version
     * of GenNode (see gtpo::ProtoSerializer documentation for more informations).
     *
     * \throw gtpo::bad_topology_error with an error description if insertion fails.
     */
    virtual WeakNode    createNode( const std::string& className ) noexcept( false );

    /*! \brief Insert a node created outside of GTpo into the graph.
     *
     * If your nodes must be created outside of GTpo (ie not with the createNode() method),
     * the only way of giving node ownership to GTpo is trought the insertNode method.
     * Example of a node created from a QML component:
     * \code
     *  qan::Node* node = static_cast< qan::Node* >( createFromQmlComponent( nodeComponent ) );
     *  if ( node != nullptr )
     *    GenGraph<QGraphcConfig>::insertNode( std::shared_ptr<qan::Node>{node} );
     * \endcode
     * \throw gtpo::bad_topology_error with an error description if insertion fails.
     */
    auto    insertNode( SharedNode node ) noexcept( false ) -> WeakNode;

    /*! \brief Remove node \c node from graph.
     *
     * Complexity depend on Config::NodeContainer.
     * \throw gtpo::bad_topology_error if node can't be removed (or node is not valid).
     */
    auto    removeNode( WeakNode weakNode ) noexcept( false ) -> void;

    //! Return the number of nodes actually registered in graph.
    auto    getNodeCount( ) const -> Size { return _nodes.size(); }
    //! Return the number of root nodes (actually registered in graph)ie nodes with a zero in degree).
    auto    getRootNodeCount( ) const -> Size { return _rootNodes.size(); }

    /*! \brief Install a given \c node in the root node cache.
     *
     * This method should not be directly used by an end user until you have deeply
     * modified graph topology with non gtpo::GenGraph<> methods.
     *
     * \throw gtpo::bad_topology_error if \c node in degree is different from 0.
     */
    auto    installRootNode( WeakNode node ) noexcept( false ) -> void;
    /*! \brief Test if a given \c node is a root node.
     *
     * This method is safer than testing node->getInDegree()==0, since it check
     * \c node in degree and its presence in the internal root node cache.
     *
     * \return true if \c node is a root node, false otherwise.
     * \throw gtpo::bad_topology_error if there is a graph cohenrency problem (ie node is in the root node cache but has a zero in degree).
     */
    auto    isRootNode( WeakNode node ) const noexcept( false ) -> bool;

    //! Use fast search container to find if a given \c node is part of this graph.
    auto    contains( WeakNode node ) const noexcept -> bool;

    //! Graph main nodes container.
    inline auto     getNodes() const -> const SharedNodes& { return _nodes; }
    //! Return a const begin iterator over graph SharedNode nodes.
    inline auto     begin( ) const -> typename SharedNodes::const_iterator { return _nodes.begin( ); }
    //! Return a const begin iterator over graph SharedNode nodes.
    inline auto     end( ) const -> typename SharedNodes::const_iterator { return _nodes.end( ); }

    //! Return a const begin iterator over graph SharedNode nodes.
    inline auto     cbegin() const -> typename SharedNodes::const_iterator { return _nodes.cbegin(); }
    //! Return a const end iterator over graph SharedNode nodes.
    inline auto     cend() const -> typename SharedNodes::const_iterator { return _nodes.cend(); }

private:
    SharedNodes         _nodes;
    WeakNodes           _rootNodes;
    WeakNodesSearch     _nodesSearch;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Graph Edge Management *///---------------------------------------
    //@{
public:
    /*! \brief Create a directed edge between \c source and \c destination node, then insert it into the graph.
     *
     * Complexity is O(1).
     * \return the inserted edge (if an error occurs edge == false and gtpo::bad_topology_error is thrown).
     * \throw a gtpo::bad_topology_error if creation fails (either \c source or \c destination does not exists).
     */
    auto        createEdge( WeakNode source, WeakNode destination ) noexcept( false ) -> WeakEdge;

    /*! \brief Create a directed hyper edge between \c source node and \c destination edge, then insert it into the graph.
     *
     * Complexity is O(1).
     * \return the inserted edge (if an error occurs edge == false and gtpo::bad_topology_error is thrown).
     * \throw a gtpo::bad_topology_error if creation fails (either \c source or \c destination does not exists).
     */
    auto        createEdge( WeakNode source, WeakEdge destination ) noexcept( false ) -> WeakEdge;

    /*! \brief Create a directed edge of a given \c className between \c source and \c destination node and insert it into the graph.
     *
     * Do not use this method until you are writing a serializer input functor with complex edge virtual hierarchy.
     *
     * Complexity is O(1).
     * \return the inserted edge (if an error occurs a gtpo::bad_topology_error is thrown).
     * \throw a gtpo::bad_topology_error if creation fails (either \c source or \c destination does not exists).
     */
    virtual WeakEdge    createEdge( const std::string& className, WeakNode source, WeakNode destination ) noexcept( false );

    //! Create a restricted directed hyperedge from \c source node to \c destination edge.
    virtual WeakEdge    createEdge( const std::string& className, WeakNode source, WeakEdge destination ) noexcept( false );

    /*! \brief Insert a directed edge created outside of GTpo into the graph.
     *
     * \param edge must have a valid source and destination set otherwise a bad topology exception will be thrown.
     * \sa insertNode()
     * \throw gtpo::bad_topology_error with an error description if insertion fails.
     */
    auto        insertEdge( SharedEdge edge ) noexcept( false ) -> WeakEdge;

    /*! \brief Remove first directed edge found between \c source and \c destination node.
     *
     * If the current GenGraph<> Config::EdgeContainer and Config::NodeContainer allow parrallel edges support, the first
     * edge found between \c source and \c destination will be removed.
     *
     * Complexity is O(edge count) at worst.
     * \throw a gtpo::bad_topology_error if suppression fails (either \c source or \c destination or edge does not exists).
     */
    auto        removeEdge( WeakNode source, WeakNode destination ) noexcept( false ) -> void;

    /*! \brief Remove all directed edge between \c source and \c destination node.
     *
     * If the current GenGraph<> Config::EdgeContainer and Config::NodeContainer allow parrallel edges support, the first
     * edge found between \c source and \c destination will be removed.
     *
     * Worst case complexity is O(edge count).
     * \throw a gtpo::bad_topology_error if suppression fails (either \c source or \c destination or edge does not exists).
     */
    auto        removeAllEdges( WeakNode source, WeakNode destination ) noexcept( false ) -> void;

    /*! \brief Remove directed edge \c edge.
     *
     * Worst case complexity is O(edge count).
     * \throw a gtpo::bad_topology_error if suppression fails (\c edge does not exists).
     */
    auto        removeEdge( WeakEdge edge ) noexcept( false ) -> void;

    /*! \brief Look for the first directed edge between \c source and \c destination and return it.
     *
     * Worst case complexity is O(edge count).
     * \return A shared reference on edge, en empty shared reference otherwise (result == false).
     * \throw noexcept.
     */
    auto        findEdge( WeakNode source, WeakNode destination ) const noexcept -> WeakEdge;
    /*! \brief Test if a directed edge exists between nodes \c source and \c destination.
     *
     * This method only test a 1 degree relationship (ie a direct edge between \c source
     * and \c destination). Worst case complexity is O(edge count).
     * \throw noexcept.
     */
    auto        hasEdge( WeakNode source, WeakNode destination ) const noexcept -> bool;
    /*! \brief Look for the first directed restricted hyper edge between \c source node and \c destination edge and return it.
     *
     * Worst case complexity is O(edge count).
     * \return A shared reference on edge, en empty shared reference otherwise (result == false).
     * \throw noexcept.
     */
    auto        findEdge( WeakNode source, WeakEdge destination ) const noexcept -> WeakEdge;
    /*! \brief Test if a directed restricted hyper edge exists between nodes \c source and \c destination.
     *
     * This method only test a 1 degree relationship (ie a direct edge between \c source
     * and \c destination). Worst case complexity is O(edge count).
     * \throw noexcept.
     */
    auto        hasEdge( WeakNode source, WeakEdge destination ) const noexcept -> bool;

    //! Return the number of edges currently existing in graph.
    auto        getEdgeCount( ) const noexcept -> unsigned int { return static_cast<int>( _edges.size() ); }
    /*! \brief Return the number of (parallel) directed edges between nodes \c source and \c destination.
     *
     * Graph EdgeContainer should support multiple insertions (std::vector, std::list) to enable
     * parrallel edge support, otherwise, getEdgeCount() will always return 1 or 0.
     *
     * This method only test a 1 degree relationship (ie a direct edge between \c source
     * and \c destination). Worst case complexity is O(edge count).
     * \throw no GTpo exception (might throw a std::bad_weak_ptr).
     */
    auto        getEdgeCount( WeakNode source, WeakNode destination ) const noexcept( false ) -> unsigned int;

    //! Use fast search container to find if a given \c edge is part of this graph.
    auto        contains( WeakEdge edge ) const noexcept -> bool;

    //! Graph main edges container.
    inline auto getEdges() const noexcept -> const SharedEdges& { return _edges; }
private:
    SharedEdges     _edges;
    WeakEdgesSearch _edgesSearch;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Graph Group Management *///--------------------------------------
    //@{
public:
    /*! \brief Create a new node group with class name \c className and insert it into the graph.
     *
     * Complexity is O(1).
     * \arg className desired class name for the create node group (default to gtpo::Group).
     * \return the inserted group (if an error occurs a gtpo::bad_topology_error is thrown).
     * \throw a gtpo::bad_topology_error if insertion fails.
     */
    auto            createGroup( ) noexcept( false ) -> WeakGroup;

    /*! \brief Used for serialization purposes, create a new group with a given class name \c className and insert it into the graph.
     *
     * Complexity is O(1).
     * \arg className desired class name for the create node group (default to gtpo::Group).
     * \return the inserted group (if an error occurs a gtpo::bad_topology_error is thrown).
     * \throw a gtpo::bad_topology_error if insertion fails.
     */
    virtual WeakGroup   createGroup( const std::string& className ) noexcept( false );

    /*! \brief Insert a node group into the graph.
     *
     * \throw gtpo::bad_topology_error with an error description if insertion fails.
     */
    auto            insertGroup( SharedGroup group ) noexcept( false ) -> WeakGroup;

    /*! \brief Remove node group \c group.
     *
     * Group content (ie group's nodes) are not removed from the graph, but ungrouped (ie moved from
     * the group to the graph).
     *
     * Worst case complexity is O(group count).
     * \throw a gtpo::bad_topology_error if suppression fails (\c group does not exists).
     */
    auto            removeGroup( WeakGroup group ) noexcept( false ) -> void;

    //! Return true if a given group \c group is registered in the graph.
    auto            hasGroup( const WeakGroup& group ) const -> bool;

    //! Return the number of edges currently existing in graph.
    inline auto     getGroupCount( ) const noexcept -> int { return static_cast<int>( _groups.size() ); }

    //! Graph main edges container.
    inline auto     getGroups() const noexcept -> const SharedGroups& { return _groups; }
private:
    SharedGroups    _groups;
    //@}
    //-------------------------------------------------------------------------
};

} // ::gtpo

#include "./gtpoGenGraph.hpp"

#endif // gtpoGenGraph_h
