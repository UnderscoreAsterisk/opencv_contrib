#include "precomp.hpp"
#include "kinfu_frame.hpp" // for ptype

namespace cv {
namespace dynafu {

WarpField::WarpField(): nodes()
{
}

std::vector<Ptr<WarpNode> > WarpField::getNodes()
{
    return nodes;
}

void WarpField::updateNodesFromPoints(InputArray _points, float resolution)
{
    // Build an index of points
    Mat m = _points.getMat();

    std::vector<float> points_vec; 
    
    for(int i = 0; i < m.size().height; i++)
    {
        points_vec.push_back(m.at<float>(i, 0));
        points_vec.push_back(m.at<float>(i, 1));
        points_vec.push_back(m.at<float>(i, 2));
    }

    ::flann::Matrix<float> points_matrix(&points_vec[0], m.size().height, 3);

    ::flann::KDTreeSingleIndex<::flann::L2_Simple <float> > searchIndex(points_matrix);
    
    searchIndex.buildIndex();

    std::vector<bool> validIndex;

    removeSupported(searchIndex, validIndex);

    subsampleIndex(searchIndex, validIndex, resolution);

}


void WarpField::removeSupported(::flann::KDTreeSingleIndex<::flann::L2_Simple<float> >& ind, std::vector<bool>& validInd)
{
    
    std::vector<bool> validIndex(ind.size(), true);

    for(WarpNode* n: nodes)
    {
        float point_array[] = {n->pos.x, n->pos.y, n->pos.y};
        ::flann::Matrix<float> query(&point_array[0], 1, 3);

        std::vector< std::vector<int> > indices_vec;
        std::vector<std::vector<float> > dists_vec;

        ind.radiusSearch(query, indices_vec, dists_vec, n->radius, ::flann::SearchParams());
        
        for(auto vec: indices_vec)
        {
            for(auto i: vec)
            {
                ind.removePoint(i);
                validIndex[i] = false;
            }
        }

        ind.buildIndex();

    }

    validInd = validIndex;

}

void WarpField::subsampleIndex(::flann::KDTreeSingleIndex<::flann::L2_Simple<float> >& ind, std::vector<bool>& validIndex, float res)
{
    for(size_t i = 0; i < validIndex.size(); i++)
    {
        if(!validIndex[i])
            continue;

        float* pt = ind.getPoint(i);
        float query_pts[] = {pt[0], pt[1], pt[2]};
        ::flann::Matrix<float> query(query_pts, 1, 3);

        std::vector<std::vector<int> > indices_vec;
        std::vector<std::vector<float> > dist_vec;
        ind.radiusSearch(query, indices_vec, dist_vec, res, ::flann::SearchParams());

        appendNodeFromCluster(res, Point3f(pt[0], pt[1], pt[2]));
        
        validIndex[i] = false;
        for(auto vec: indices_vec)
            for(auto pi: vec)
                validIndex[pi] = false;
    }      
}

void WarpField::appendNodeFromCluster(float res, Point3f p)
{
    Ptr<WarpNode> wn = new WarpNode;

    wn->pos = p;
    wn->radius = res;
    nodes.push_back(wn);
}

} // namepsace dynafu
} // namespace cv
