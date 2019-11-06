/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/vision/VisionIncludes.h"

namespace Saiga
{
/**
 * Identical to OpenCV's Keypoint struct except it uses
 * Eigen Points and no 'class_id' member.
 * @brief The Keypoint struct
 */
/*
struct Point
{
int x;
int y;

Point() : x(0), y(0) {}

Point(int _x, int _y) : x(_x), y(_y) {}

bool inline operator==(const Point &other) const
{
   return x == other.x && y == other.y;
}


template <typename T> inline
friend Point operator*(const T s, const Point& pt)
{
   return Point(pt.x*s, pt.y*s);
}

template <typename T> inline
friend void operator*=(Point& pt, const T s)
{
   pt.x*=s;
   pt.y*=s;
}

friend std::ostream& operator<<(std::ostream& os, const Point& pt)
{
   os << "[" << pt.x << "," << pt.y << "]";
   return os;
}
};
 */
template <typename T = float>
class KeyPoint
{
   public:
    using Vec2 = Eigen::Matrix<T, 2, 1>;


    Vec2 point;

    T size;
    T angle;
    T response;
    int octave;

    KeyPoint() : point(), size(0), angle(-1), response(0), octave(0) {}

    explicit KeyPoint(Vec2 _pt, T _size = 0, T _angle = -1, T _response = 0, int _octave = 0)
        : point(_pt), size(_size), angle(_angle), response(_response), octave(_octave)
    {
    }

    KeyPoint(T _x, T _y, T _size = 0, T _angle = -1, T _response = 0, int _octave = 0)
        : point(_x, _y), size(_size), angle(_angle), response(_response), octave(_octave)
    {
    }

    bool operator==(const KeyPoint& other) const
    {
        return point == other.point && size == other.size && angle == other.angle && response == other.response &&
               octave == other.octave;
    }

    friend std::ostream& operator<<(std::ostream& os, const KeyPoint& kpt)
    {
        os << kpt.point << ": size=" << kpt.size << ", angle=" << kpt.angle << ", response=" << kpt.response
           << ", octave=" << kpt.octave;
        return os;
    }
};

// Some common feature descriptors
using DescriptorORB  = std::array<uint64_t, 4>;
using DescriptorSIFT = std::array<float, 128>;



#ifndef WIN32
// use the popcnt instruction
// this will be the fastest implementation if it is available
// more here: https://github.com/kimwalisch/libpopcnt
inline uint32_t popcnt(uint32_t x)
{
    __asm__("popcnt %1, %0" : "=r"(x) : "0"(x));
    return x;
}
inline uint64_t popcnt(uint64_t x)
{
    __asm__("popcnt %1, %0" : "=r"(x) : "0"(x));
    return x;
}
#else
// Bit count function got from:
// http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan
inline uint32_t popcnt(uint32_t v)
{
    v = v - ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}
inline uint64_t popcnt(uint64_t v)
{
    v = v - ((v >> 1) & (uint64_t) ~(uint64_t)0 / 3);
    v = (v & (uint64_t) ~(uint64_t)0 / 15 * 3) + ((v >> 2) & (uint64_t) ~(uint64_t)0 / 15 * 3);
    v = (v + (v >> 4)) & (uint64_t) ~(uint64_t)0 / 255 * 15;
    return (uint64_t)(v * ((uint64_t) ~(uint64_t)0 / 255)) >> (sizeof(uint64_t) - 1) * CHAR_BIT;
}
#endif

// Compute the hamming distance between the two descriptors
// Same implementation as ORB SLAM
// http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
inline int distance(const DescriptorORB& a, const DescriptorORB& b)
{
    int dist = 0;
    for (int i = 0; i < (int)a.size(); i++)
    {
        auto v = a[i] ^ b[i];
        // TODO: if this is really a bottleneck we can also use AVX-2
        // to gain around 25% more performance
        // according to this source:
        // https://github.com/kimwalisch/libpopcnt
        dist += popcnt(v);
    }

    return dist;
}



// Compute the euclidean distance between the descriptors
inline float distance(const DescriptorSIFT& a, const DescriptorSIFT& b)
{
    float sumSqr = 0;
    for (int i = 0; i < 128; ++i)
    {
        auto c = a[i] - b[i];
        sumSqr += c * c;
    }
    return sqrt(sumSqr);
}


template <typename T>
struct MeanMatcher
{
    inline int bestDescriptorFromArray(const std::vector<T>& descriptors)
    {
        static_assert(std::is_same<T, DescriptorORB>::value, "Only implemented for ORB so far.");
        if (descriptors.size() == 0) return -1;
        // Compute distances between them
        size_t N = descriptors.size();
        std::vector<std::vector<int>> Distances(N, std::vector<int>(N));

        for (size_t i = 0; i < N; i++)
        {
            Distances[i][i] = 0;
            for (size_t j = i + 1; j < N; j++)
            {
                int distij      = distance(descriptors[i], descriptors[j]);
                Distances[i][j] = distij;
                Distances[j][i] = distij;
            }
        }

        // Take the descriptor with least median distance to the rest
        int BestMedian = INT_MAX;
        int BestIdx    = 0;
        for (size_t i = 0; i < N; i++)
        {
            // vector<int> vDists(Distances[i],Distances[i]+N);
            auto& vDists = Distances[i];
            sort(vDists.begin(), vDists.end());
            int median = vDists[0.5 * (N - 1)];

            if (median < BestMedian)
            {
                BestMedian = median;
                BestIdx    = i;
            }
        }
        return BestIdx;
    }
};

template <typename T>
struct BruteForceMatcher
{
    using DistanceType = int;


    template <typename _InputIterator>
    void match(_InputIterator first1, int n, _InputIterator first2, int m)
    {
        distances.resize(n, m);
        for (auto i : Range(0, n))
        {
            auto d2 = first2;
            for (auto j : Range(0, m))
            {
                distances(i, j) = distance(*first1, *d2);
                ++d2;
            }
            ++first1;
        }

        int sum = distances.sum();
        std::cout << "distance sum: " << sum << " avg: " << double(sum) / (n * m) << std::endl;
    }

    template <typename _InputIterator>
    void matchKnn2(_InputIterator first1, int n, _InputIterator first2, int m)
    {
        knn2.resize(n, 2);
        for (auto i : Range(0, n))
        {
            // init best to infinity distance
            knn2(i, 0) = {1000, -1};
            knn2(i, 1) = knn2(i, 0);

            auto d2 = first2;
            for (auto j : Range(0, m))
            {
                auto dis = distance(*first1, *d2);

                if (dis < knn2(i, 0).first)
                {
                    // set second best to old best
                    knn2(i, 1) = knn2(i, 0);
                    // create new best
                    knn2(i, 0).first  = dis;
                    knn2(i, 0).second = j;
                }
                else if (dis < knn2(i, 1).first)
                {
                    // override second best
                    knn2(i, 1).first  = dis;
                    knn2(i, 1).second = j;
                }
                ++d2;
            }
            ++first1;
        }
    }


    /**
     * Filter matches by ratio test and threshold.
     * You must have used the knn2 method above before!
     */
    int filterMatches(DistanceType threshold, float ratioThreshold)
    {
        matches.clear();
        matches.reserve(knn2.rows());

        for (auto i : Range<int>(0, knn2.rows()))
        {
            // the best distance is still larger than the threshold
            if (knn2(i, 0).first > threshold) continue;

            //            float ratio = float(knn2(i, 0).first) / float(knn2(i, 1).first);
            //            std::cout << "for " << i << " best/second: " << knn2(i, 0).first << "/" << knn2(i, 1).first <<
            //            " " << ratio
            //                      << std::endl;

            if (float(knn2(i, 0).first) > float(knn2(i, 1).first) * ratioThreshold) continue;

            matches.push_back({i, knn2(i, 0).second});
        }
        return matches.size();
    }


    Eigen::Matrix<DistanceType, -1, -1, Eigen::RowMajor> distances;

    // contains the matches index + the distance
    Eigen::Matrix<std::pair<DistanceType, int>, -1, 2, Eigen::RowMajor> knn2;

    std::vector<std::pair<int, int>> matches;
};

}  // namespace Saiga
