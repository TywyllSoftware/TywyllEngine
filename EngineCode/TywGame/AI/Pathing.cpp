#include <stdafx.h>
#include <TywLib\math\GLXMath.h>

#include "AI\Pathing.h"


//--------------------------------------------------------------------------------------------------------
// PathingNode
//--------------------------------------------------------------------------------------------------------
//GCC_MEMORYPOOL_DEFINITION(PathingNode);
//GCC_MEMORYPOOL_AUTOINIT(PathingNode, 128);  // there are currently 81 pathing nodes generated; if you make more, you should increase this number

void PathingNode::AddArc(PathingArc* pArc)
{
	assert(pArc);
	m_arcs.push_back(pArc);
}

void PathingNode::GetNeighbors(PathingNodeList& outNeighbors)
{
	for (PathingArcList::iterator it = m_arcs.begin(); it != m_arcs.end(); ++it)
	{
		PathingArc* pArc = *it;
		outNeighbors.push_back(pArc->GetNeighbor(this));
	}
}

float PathingNode::GetCostFromNode(PathingNode* pFromNode)
{
	assert(pFromNode);
	PathingArc* pArc = FindArc(pFromNode);
	assert(pArc);
	glx::vec3<float> diff = pFromNode->GetPos() - m_pos;
	return pArc->GetWeight() * GLXVec3Length(&diff);
}

PathingArc* PathingNode::FindArc(PathingNode* pLinkedNode)
{
	assert(pLinkedNode);

	for (PathingArcList::iterator it = m_arcs.begin(); it != m_arcs.end(); ++it)
	{
		PathingArc* pArc = *it;
		if (pArc->GetNeighbor(this) == pLinkedNode)
			return pArc;
	}
	return NULL;
}


//--------------------------------------------------------------------------------------------------------
// PathingArc
//--------------------------------------------------------------------------------------------------------
void PathingArc::LinkNodes(PathingNode* pNodeA, PathingNode* pNodeB)
{
	assert(pNodeA);
	assert(pNodeB);

	m_pNodes[0] = pNodeA;
	m_pNodes[1] = pNodeB;
}

PathingNode* PathingArc::GetNeighbor(PathingNode* pMe)
{
	assert(pMe);

	if (m_pNodes[0] == pMe)
		return m_pNodes[1];
	else
		return m_pNodes[0];
}


//--------------------------------------------------------------------------------------------------------
// PathPlan
//--------------------------------------------------------------------------------------------------------
bool PathPlan::CheckForNextNode(const glx::vec3<float>& pos)
{
	if (m_index == m_path.end())
		return false;

	glx::vec3<float> diff = pos - (*m_index)->GetPos();

	// DEBUG dump target orientation
	//wchar_t str[64];  // I'm sure this is overkill
	//memset(str,0,sizeof(wchar_t));
	//swprintf_s(str,64,_T("distance: %f\n"),diff.Length());
	//GCC_LOG("AI", str);
	// end DEBUG

	if (GLXVec3Length(&diff) <= (*m_index)->GetTolerance())
	{
		++m_index;
		return true;
	}
	return false;
}

bool PathPlan::CheckForEnd(void)
{
	if (m_index == m_path.end())
		return true;
	return false;
}

void PathPlan::AddNode(PathingNode* pNode)
{
	assert(pNode);
	m_path.push_front(pNode);
}


//--------------------------------------------------------------------------------------------------------
// PathPlanNode
//--------------------------------------------------------------------------------------------------------
PathPlanNode::PathPlanNode(PathingNode* pNode, PathPlanNode* pPrevNode, PathingNode* pGoalNode)
{
	assert(pNode);

	m_pPathingNode = pNode;
	m_pPrev = pPrevNode;  // NULL is a valid value, though it should only be NULL for the start node
	m_pGoalNode = pGoalNode;
	m_closed = false;
	UpdateHeuristics();
}

void PathPlanNode::UpdatePrevNode(PathPlanNode* pPrev)
{
	assert(pPrev);
	m_pPrev = pPrev;
	UpdateHeuristics();
}

void PathPlanNode::UpdateHeuristics(void)
{
	// total cost (g)
	if (m_pPrev)
		m_goal = m_pPrev->GetGoal() + m_pPathingNode->GetCostFromNode(m_pPrev->GetPathingNode());
	else
		m_goal = 0;

	// heuristic (h)
	glx::vec3<float> diff = m_pPathingNode->GetPos() - m_pGoalNode->GetPos();
	m_heuristic = GLXVec3Length(&diff);

	// cost to goal (f)
	m_fitness = m_goal + m_heuristic;
}


//--------------------------------------------------------------------------------------------------------
// AStar
//--------------------------------------------------------------------------------------------------------
AStar::AStar(void)
{
	m_pStartNode = NULL;
	m_pGoalNode = NULL;
}

AStar::~AStar(void)
{
	Destroy();
}

void AStar::Destroy(void)
{
	// destroy all the PathPlanNode objects and clear the map
	for (PathingNodeToPathPlanNodeMap::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
		delete it->second;
	m_nodes.clear();

	// clear the open set
	m_openSet.clear();

	// clear the start & goal nodes
	m_pStartNode = NULL;
	m_pGoalNode = NULL;
}


//
// AStar::operator()					- Chapter 18, page 638
//
PathPlan* AStar::operator()(PathingNode* pStartNode, PathingNode* pGoalNode)
{
	assert(pStartNode);
	assert(pGoalNode);

	// if the start and end nodes are the same, we're close enough to b-line to the goal
	if (pStartNode == pGoalNode)
		return NULL;

	// set our members
	m_pStartNode = pStartNode;
	m_pGoalNode = pGoalNode;

	// The open set is a priority queue of the nodes to be evaluated.  If it's ever empty, it means 
	// we couldn't find a path to the goal.  The start node is the only node that is initially in 
	// the open set.
	AddToOpenSet(m_pStartNode, NULL);

	while (!m_openSet.empty())
	{
		// grab the most likely candidate
		PathPlanNode* pNode = m_openSet.front();

		// If this node is our goal node, we've successfully found a path.
		if (pNode->GetPathingNode() == m_pGoalNode)
			return RebuildPath(pNode);

		// we're processing this node so remove it from the open set and add it to the closed set
		m_openSet.pop_front();
		AddToClosedSet(pNode);

		// get the neighboring nodes
		PathingNodeList neighbors;
		pNode->GetPathingNode()->GetNeighbors(neighbors);

		// loop though all the neighboring nodes and evaluate each one
		for (PathingNodeList::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
		{
			PathingNode* pNodeToEvaluate = *it;

			// Try and find a PathPlanNode object for this node.
			PathingNodeToPathPlanNodeMap::iterator findIt = m_nodes.find(pNodeToEvaluate);

			// If one exists and it's in the closed list, we've already evaluated the node.  We can
			// safely skip it.
			if (findIt != m_nodes.end() && findIt->second->IsClosed())
				continue;

			// figure out the cost for this route through the node
			float costForThisPath = pNode->GetGoal() + pNodeToEvaluate->GetCostFromNode(pNode->GetPathingNode());
			bool isPathBetter = false;

			// Grab the PathPlanNode if there is one.
			PathPlanNode* pPathPlanNodeToEvaluate = NULL;
			if (findIt != m_nodes.end())
				pPathPlanNodeToEvaluate = findIt->second;

			// No PathPlanNode means we've never evaluated this pathing node so we need to add it to 
			// the open set, which has the side effect of setting all the heuristic data.  It also 
			// means that this is the best path through this node that we've found so the nodes are 
			// linked together (which is why we don't bother setting isPathBetter to true; it's done
			// for us in AddToOpenSet()).
			if (!pPathPlanNodeToEvaluate)
				pPathPlanNodeToEvaluate = AddToOpenSet(pNodeToEvaluate, pNode);

			// If this node is already in the open set, check to see if this route to it is better than
			// the last.
			else if (costForThisPath < pPathPlanNodeToEvaluate->GetGoal())
				isPathBetter = true;

			// If this path is better, relink the nodes appropriately, update the heuristics data, and
			// reinsert the node into the open list priority queue.
			if (isPathBetter)
			{
				pPathPlanNodeToEvaluate->UpdatePrevNode(pNode);
				ReinsertNode(pPathPlanNodeToEvaluate);
			}
		}
	}

	// If we get here, there's no path to the goal.
	return NULL;
}

PathPlanNode* AStar::AddToOpenSet(PathingNode* pNode, PathPlanNode* pPrevNode)
{
	assert(pNode);

	// create a new PathPlanNode if necessary
	PathingNodeToPathPlanNodeMap::iterator it = m_nodes.find(pNode);
	PathPlanNode* pThisNode = NULL;
	if (it == m_nodes.end())
	{
		pThisNode = TYW_NEW PathPlanNode(pNode, pPrevNode, m_pGoalNode);
		m_nodes.insert(std::make_pair(pNode, pThisNode));
	}
	else
	{
		//GCC_WARNING("Adding existing PathPlanNode to open set");
		pThisNode = it->second;
		pThisNode->SetClosed(false);
	}

	// now insert it into the priority queue
	InsertNode(pThisNode);

	return pThisNode;
}

void AStar::AddToClosedSet(PathPlanNode* pNode)
{
	assert(pNode);
	pNode->SetClosed();
}

//
// AStar::InsertNode					- Chapter 17, page 636
//
void AStar::InsertNode(PathPlanNode* pNode)
{
	assert(pNode);

	// just add the node if the open set is empty
	if (m_openSet.empty())
	{
		m_openSet.push_back(pNode);
		return;
	}

	// otherwise, perform an insertion sort	
	PathPlanNodeList::iterator it = m_openSet.begin();
	PathPlanNode* pCompare = *it;
	while (pCompare->IsBetterChoiceThan(pNode))
	{
		++it;

		if (it != m_openSet.end())
			pCompare = *it;
		else
			break;
	}
	m_openSet.insert(it, pNode);
}

void AStar::ReinsertNode(PathPlanNode* pNode)
{
	assert(pNode);

	for (PathPlanNodeList::iterator it = m_openSet.begin(); it != m_openSet.end(); ++it)
	{
		if (pNode == (*it))
		{
			m_openSet.erase(it);
			InsertNode(pNode);
			return;
		}
	}

	// if we get here, the node was never in the open set to begin with
	//GCC_WARNING("Attemping to reinsert node that was never in the open list");
	InsertNode(pNode);
}

PathPlan* AStar::RebuildPath(PathPlanNode* pGoalNode)
{
	assert(pGoalNode);

	PathPlan* pPlan = TYW_NEW PathPlan;

	PathPlanNode* pNode = pGoalNode;
	while (pNode)
	{
		pPlan->AddNode(pNode->GetPathingNode());
		pNode = pNode->GetPrev();
	}

	return pPlan;
}


//--------------------------------------------------------------------------------------------------------
// PathingGraph
//--------------------------------------------------------------------------------------------------------
void PathingGraph::DestroyGraph(void)
{
	// destroy all the nodes
	for (PathingNodeVec::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
		delete (*it);
	m_nodes.clear();

	// destroy all the arcs
	for (PathingArcList::iterator it = m_arcs.begin(); it != m_arcs.end(); ++it)
		delete (*it);
	m_arcs.clear();
}

PathingNode* PathingGraph::FindClosestNode(const glx::vec3<float>& pos)
{
	// This is a simple brute-force O(n) algorithm that could be made a LOT faster by utilizing
	// spatial partitioning, like an octree (or quadtree for flat worlds) or something similar.
	PathingNode* pClosestNode = m_nodes.front();
	float length = FLT_MAX;
	for (PathingNodeVec::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		PathingNode* pNode = *it;
		glx::vec3<float> diff = pos - pNode->GetPos();
		if (GLXVec3Length(&diff) < length)
		{
			pClosestNode = pNode;
			length = GLXVec3Length(&diff);
		}
	}

	return pClosestNode;
}

PathingNode* PathingGraph::FindFurthestNode(const glx::vec3<float>& pos)
{
	// This is a simple brute-force O(n) algorithm that could be made a LOT faster by utilizing
	// spatial partitioning, like an octree (or quadtree for flat worlds) or something similar.
	PathingNode* pFurthestNode = m_nodes.front();
	float length = 0;
	for (PathingNodeVec::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		PathingNode* pNode = *it;
		glx::vec3<float> diff = pos - pNode->GetPos();
		if (GLXVec3Length(&diff) > length)
		{
			pFurthestNode = pNode;
			length = GLXVec3Length(&diff);
		}
	}

	return pFurthestNode;
}

PathingNode* PathingGraph::FindRandomNode(void)
{
	// cache this since it's not guaranteed to be constant time
	unsigned int numNodes = (unsigned int)m_nodes.size();

	// choose a random node
	unsigned int node; // = g_pApp->m_pGame->GetRNG().Random(numNodes);

	// if we're in the lower half of the node list, start from the bottom
	if (node <= numNodes / 2)
	{
		PathingNodeVec::iterator it = m_nodes.begin();
		for (unsigned int i = 0; i < node; i++)
			++it;
		return (*it);
	}

	// otherwise, start from the top
	else
	{
		PathingNodeVec::iterator it = m_nodes.end();
		for (unsigned int i = numNodes; i >= node; i--)
			--it;
		return (*it);
	}
}

PathPlan* PathingGraph::FindPath(const glx::vec3<float>& startPoint, const glx::vec3<float>& endPoint)
{
	// Find the closest nodes to the start and end points.  There really should be some ray-casting 
	// to ensure that we can actually make it to the closest node, but this is good enough for now.
	PathingNode* pStart = FindClosestNode(startPoint);
	PathingNode* pGoal = FindClosestNode(endPoint);
	return FindPath(pStart, pGoal);
}

PathPlan* PathingGraph::FindPath(const glx::vec3<float>& startPoint, PathingNode* pGoalNode)
{
	PathingNode* pStart = FindClosestNode(startPoint);
	return FindPath(pStart, pGoalNode);
}

PathPlan* PathingGraph::FindPath(PathingNode* pStartNode, const glx::vec3<float>& endPoint)
{
	PathingNode* pGoal = FindClosestNode(endPoint);
	return FindPath(pStartNode, pGoal);
}

PathPlan* PathingGraph::FindPath(PathingNode* pStartNode, PathingNode* pGoalNode)
{
	// find the best path using an A* search algorithm
	AStar aStar;
	return aStar(pStartNode, pGoalNode);
}

void PathingGraph::BuildTestGraph(void)
{
	// this should never occur, but better safe than sorry
	if (!m_nodes.empty())
		DestroyGraph();

	// keep from reallocating and copying the array
	m_nodes.reserve(81);

	// Create a simple grid of nodes.  Using these hard-coded values is a bit hacky but it's okay 
	// because this is just a debug function.
	int index = 0;  // this is used to keep track of the node we just inserted so we can link it to adjacent nodes
	for (float x = -45.0f; x < 45.0f; x += 10.0f)
	{
		for (float z = -45.0f; z < 45.0f; z += 10.0f)
		{
			// add the new node
			PathingNode* pNode = new PathingNode(glx::vec3<float>(x, 0, z));
			m_nodes.push_back(pNode);

			// link it to the previous node
			int tempNode = index - 1;
			if (tempNode >= 0)
				LinkNodes(m_nodes[tempNode], pNode);

			// link it to the node above it
			tempNode = index - 9;  // reusing tempNode
			if (tempNode >= 0)
				LinkNodes(m_nodes[tempNode], pNode);

			index++;
		}
	}
}

void PathingGraph::LinkNodes(PathingNode* pNodeA, PathingNode* pNodeB)
{
	assert(pNodeA);
	assert(pNodeB);

	PathingArc* pArc = TYW_NEW PathingArc;
	pArc->LinkNodes(pNodeA, pNodeB);
	pNodeA->AddArc(pArc);
	pNodeB->AddArc(pArc);
	m_arcs.push_back(pArc);
}


//--------------------------------------------------------------------------------------------------------
// Global functions
//--------------------------------------------------------------------------------------------------------
PathingGraph* CreatePathingGraph(void)
{
	PathingGraph* pPathingGraph = TYW_NEW PathingGraph;
	pPathingGraph->BuildTestGraph();
	return pPathingGraph;
}

