//---------------------------------------------------------------------------
#ifndef NUMERICAL_HPP
#define NUMERICAL_HPP
#include "image/utility/basic_image.hpp"
#include "image/numerical/interpolation.hpp"
#include "image/numerical/basic_op.hpp"



namespace image
{
//---------------------------------------------------------------------------
template<typename input_iterator,typename output_iterator>
inline void gradient(input_iterator src_from,input_iterator src_to,
                     output_iterator dest_from,unsigned int src_shift,unsigned int dest_shift)
{
    input_iterator src_from2 = src_from+src_shift;
    if(dest_shift)
    {
        output_iterator dest_from_beg = dest_from;
        dest_from += dest_shift;
        std::fill(dest_from_beg,dest_from,0);
    }
    for (; src_from2 < src_to; ++src_from,++src_from2,++dest_from)
        (*dest_from) = (*src_from2) - (*src_from);

    if(dest_shift < src_shift)
        std::fill(dest_from,dest_from+(src_shift-dest_shift),0);
}
//---------------------------------------------------------------------------
template<typename PixelImageType,typename VectorImageType>
inline void gradient(const PixelImageType& src,VectorImageType& dest_,unsigned int src_shift,unsigned int dest_shift)
{
    VectorImageType dest(src.geometry());
    gradient(src.begin(),src.end(),dest.begin(),src_shift,dest_shift);
    dest.swap(dest_);
}
//---------------------------------------------------------------------------
template<typename PixelImageType,typename VectorImageType>
void gradient(const PixelImageType& src,VectorImageType& dest)
{
    dest.clear();
    dest.resize(src.geometry());
    int shift = 1;
    for (unsigned int index = 0; index < PixelImageType::dimension; ++index)
    {
        typename PixelImageType::const_iterator in_from1 = src.begin();
        typename PixelImageType::const_iterator in_from2 = src.begin()+shift;
        typename VectorImageType::iterator out_from = dest.begin();
        typename PixelImageType::const_iterator in_to = src.end();
        for (; in_from2 != in_to; ++in_from1,++in_from2,++out_from)
            (*out_from)[index] = (*in_from2 - *in_from1);
        shift *= src.geometry()[index];
    }
}
//---------------------------------------------------------------------------
template<typename PixelImageType,typename VectorImageType>
void gradient_sobel(const PixelImageType& src,VectorImageType& dest)
{
    dest.clear();
    dest.resize(src.geometry());
    int shift = 1;
    for (unsigned int index = 0; index < PixelImageType::dimension; ++index)
    {
        typename PixelImageType::const_iterator in_from1 = src.begin();
        typename PixelImageType::const_iterator in_from2 = src.begin()+shift+shift;
        typename VectorImageType::iterator out_from = dest.begin()+shift;
        typename PixelImageType::const_iterator in_to = src.end();
        for (; in_from2 != in_to; ++in_from1,++in_from2,++out_from)
            (*out_from)[index] = (*in_from2 - *in_from1);
        shift *= src.geometry()[index];
    }
}
//---------------------------------------------------------------------------
template<typename pixel_type,typename container_type,typename VectorImageType>
void gradient_multiple_sampling(const image::basic_image<pixel_type,3,container_type>& src,
                                VectorImageType& dest,
                                double line_interval = 1.0,
                                unsigned int line_sampling_num = 3,
                                unsigned int sampling_dir_num = 8)
{
}
template<typename pixel_type,typename container_type,typename VectorImageType>
void gradient_multiple_sampling(const image::basic_image<pixel_type,2,container_type>& src,
                                VectorImageType& dest,
                                double line_interval = 1.0,
                                unsigned int line_sampling_num = 3,
                                unsigned int sampling_dir_num = 8)
{
    typedef typename VectorImageType::value_type vector_type;
    std::vector<image::interpolation<image::linear_weighting,2> >
    interpo1(sampling_dir_num*line_sampling_num),interpo2(sampling_dir_num*line_sampling_num);
    std::vector<vector_type> offset_vector(sampling_dir_num*line_sampling_num);
    std::vector<double> offset_length(sampling_dir_num*line_sampling_num);

    {
        vector_type center(src.width() >> 1,src.height() >> 1);
        int center_index = (src.width() >> 1) + src.width()*(src.height() >> 1);
        for(unsigned int i = 0,index = 0; i < sampling_dir_num; ++i)
        {
            vector_type dist;
            dist[0] = std::cos(3.1415926*(double)i/(double)sampling_dir_num);
            dist[1] = std::sin(3.1415926*(double)i/(double)sampling_dir_num);
            for(unsigned int j = 1; j <= line_sampling_num; ++j,++index)
            {
                vector_type r = dist;
                r *= line_interval;
                r *= j;
                interpo1[index].get_location(src.geometry(),center+r);
                interpo2[index].get_location(src.geometry(),center-r);
                offset_vector[index] = dist;
                offset_length[index] = 2*line_interval*(double)j;
            }
        }
        for(unsigned int index = 0; index < interpo1.size(); ++index)
            for(unsigned int i = 0; i < 4; ++i)
            {
                interpo1[index].dindex[i] -= center_index;
                interpo2[index].dindex[i] -= center_index;
            }
    }

    dest.resize(src.geometry());
    for (image::pixel_index<2> iter; iter.valid(src.geometry()); iter.next(src.geometry()))
    {
        vector_type dist;
        unsigned int total_dir = 0;
        for(unsigned int index = 0; index < interpo1.size(); ++index)
        {
            if(interpo1[index].dindex[0] >= 0 &&
               interpo1[index].dindex[3] < src.size() &&
               interpo2[index].dindex[0] >= 0 &&
               interpo2[index].dindex[3] < src.size())
            {
                double value1,value2;
                interpo1[index].estimate(src,value1);
                interpo2[index].estimate(src,value2);
                value1 -= value2;
                value1 /= offset_length[index];
                dist += offset_vector[index]*value1;
                ++total_dir;
            }
            for(unsigned int i = 0; i < 4; ++i)
            {
                ++(interpo1[index].dindex[i]);
                ++(interpo2[index].dindex[i]);
            }

        }
        if(total_dir)
            dest[iter.index()] = dist/(double)total_dir;
        else
            dest[iter.index()] = vector_type();
    }
}
//---------------------------------------------------------------------------
template<typename PixelImageType,typename GradientImageType>
void gradient(const PixelImageType& src,std::vector<GradientImageType>& dest)
{
    dest.resize(PixelImageType::dimension);
    unsigned int shift = 1;
    for (unsigned int index = 0; index < PixelImageType::dimension; ++index)
    {
        dest[index].resize(src.geometry());
        gradient(src,dest[index],shift,0);
        shift *= src.geometry()[index];
    }
}


//---------------------------------------------------------------------------
// implement -1 0 1
template<typename InputImageType,typename OutputImageType>
void gradient_2x(const InputImageType& in,OutputImageType& out)
{
    gradient(in,out,2,1);
}
//---------------------------------------------------------------------------
// implement -1
//            0
//            1
template<typename InputImageType,typename OutputImageType>
void gradient_2y(const InputImageType& in,OutputImageType& out)
{
    gradient(in,out,in.width() << 1,in.width());
}
//---------------------------------------------------------------------------
template<typename InputImageType,typename OutputImageType>
void gradient_2z(const InputImageType& in,OutputImageType& out)
{
    gradient(in,out,in.geometry().plane_size() << 1,in.geometry().plane_size());
}
//---------------------------------------------------------------------------
template<typename LHType,typename RHType>
void assign_negate(LHType& lhs,const RHType& rhs)
{
    unsigned int total_size;
    total_size = (lhs.size() < rhs.size()) ? lhs.size() : rhs.size();
    typename LHType::iterator lhs_from = lhs.begin();
    typename LHType::iterator lhs_to = lhs_from + total_size;
    typename RHType::const_iterator rhs_from = rhs.begin();
    for (; lhs_from != lhs_to; ++lhs_from,++rhs_from)
        *lhs_from = -(*rhs_from);
}
//---------------------------------------------------------------------------
template<typename iterator,typename fun_type>
void apply_fun(iterator lhs_from,iterator lhs_to,fun_type fun)
{
    for (; lhs_from != lhs_to; ++lhs_from)
        *lhs_from = fun(*lhs_from);
}
//---------------------------------------------------------------------------
template<typename iterator>
void square(iterator lhs_from,iterator lhs_to)
{
    typename std::iterator_traits<iterator>::value_type tmp;
    for (; lhs_from != lhs_to; ++lhs_from)
    {
        tmp = *lhs_from;
        *lhs_from = tmp*tmp;
    }
}
//---------------------------------------------------------------------------
template<typename iterator>
void square_root(iterator lhs_from,iterator lhs_to)
{
    for (; lhs_from != lhs_to; ++lhs_from)
        *lhs_from = std::sqrt(*lhs_from);
}
//---------------------------------------------------------------------------
template<typename iterator>
void log(iterator lhs_from,iterator lhs_to)
{
    for (; lhs_from != lhs_to; ++lhs_from)
        *lhs_from = std::log(*lhs_from);
}
//---------------------------------------------------------------------------
template<typename iterator>
void absolute_value(iterator lhs_from,iterator lhs_to)
{
    for (; lhs_from != lhs_to; ++lhs_from)
        *lhs_from = std::abs(*lhs_from);
}
//---------------------------------------------------------------------------
template<typename iterator1,typename iterator2>
void add(iterator1 lhs_from,iterator1 lhs_to,iterator2 rhs_from)
{
    for (; lhs_from != lhs_to; ++lhs_from,++rhs_from)
        *lhs_from += *rhs_from;
}
//---------------------------------------------------------------------------
template<typename iterator1,typename iterator2>
void minus(iterator1 lhs_from,iterator1 lhs_to,iterator2 rhs_from)
{
    for (; lhs_from != lhs_to; ++lhs_from,++rhs_from)
        *lhs_from -= *rhs_from;
}
//---------------------------------------------------------------------------
template<typename iterator1,typename iterator2>
void multiply(iterator1 lhs_from,iterator1 lhs_to,iterator2 rhs_from)
{
    for (; lhs_from != lhs_to; ++lhs_from,++rhs_from)
        *lhs_from *= *rhs_from;
}
//---------------------------------------------------------------------------
template<typename iterator1,typename iterator2>
void divide(iterator1 lhs_from,iterator1 lhs_to,iterator2 rhs_from)
{
    for (; lhs_from != lhs_to; ++lhs_from,++rhs_from)
        *lhs_from /= *rhs_from;
}
//---------------------------------------------------------------------------
template<typename iterator1,typename value_type>
void add_constant(iterator1 lhs_from,iterator1 lhs_to,value_type value)
{
    for (; lhs_from != lhs_to; ++lhs_from)
        *lhs_from += value;
}
//---------------------------------------------------------------------------
template<typename iterator1,typename value_type>
void minus_constant(iterator1 lhs_from,iterator1 lhs_to,value_type value)
{
    for (; lhs_from != lhs_to; ++lhs_from)
        *lhs_from -= value;
}
//---------------------------------------------------------------------------
template<typename iterator1,typename value_type>
void multiply_constant(iterator1 lhs_from,iterator1 lhs_to,value_type value)
{
    for (; lhs_from != lhs_to; ++lhs_from)
        *lhs_from *= value;
}
//---------------------------------------------------------------------------
template<typename iterator1,typename value_type>
void divide_constant(iterator1 lhs_from,iterator1 lhs_to,value_type value)
{
    for (; lhs_from != lhs_to; ++lhs_from)
        *lhs_from /= value;
}
//---------------------------------------------------------------------------
// perform x <- x*pow(2,y)
template<typename iterator1,typename iterator2>
void multiply_pow(iterator1 lhs_from,iterator1 lhs_to,iterator2 rhs_from)
{
    typename std::iterator_traits<iterator1>::value_type value;
    typename std::iterator_traits<iterator1>::value_type pow;
    for (; lhs_from != lhs_to; ++lhs_from,++rhs_from)
    {
        value = *lhs_from;
        pow = *rhs_from;
        if(!value || !pow)
            continue;
        if(pow < 0)
        {
            pow = -pow;
            if(value >= 0)
                *lhs_from = (value >> pow);
            else
            {
                value = -value;
                *lhs_from = -(value >> pow);
            }
        }
        else
        {
            if(value >= 0)
                *lhs_from = (value << pow);
            else
            {
                value = -value;
                *lhs_from = -(value << pow);
            }
        }
    }
}
//---------------------------------------------------------------------------
// perform x <- x*2^pow
template<typename iterator1,typename value_type>
void multiply_pow_constant(iterator1 lhs_from,iterator1 lhs_to,value_type pow)
{
    typename std::iterator_traits<iterator1>::value_type value;
    if(pow == 0)
        return;
    if(pow > 0)
    {
        for (; lhs_from != lhs_to; ++lhs_from)
        {
            value = *lhs_from;
            if(value >= 0)
                *lhs_from = (value << pow);
            else
            {
                value = -value;
                *lhs_from = -(value << pow);
            }
        }
    }
    else
    {
        pow = -pow;
        for (; lhs_from != lhs_to; ++lhs_from)
        {
            value = *lhs_from;
            if(value >= 0)
                *lhs_from = (value >> pow);
            else
            {
                value = -value;
                *lhs_from = -(value >> pow);
            }
        }
    }
}
//---------------------------------------------------------------------------
// perform x <- x/pow(2,y)
template<typename iterator1,typename iterator2>
void divide_pow(iterator1 lhs_from,iterator1 lhs_to,iterator2 rhs_from)
{
    typename std::iterator_traits<iterator1>::value_type value;
    typename std::iterator_traits<iterator1>::value_type pow;
    for (; lhs_from != lhs_to; ++lhs_from,++rhs_from)
    {
        value = *lhs_from;
        pow = *rhs_from;
        if(!value || !pow)
            continue;
        if(pow < 0)
        {
            pow = -pow;
            if(value >= 0)
                *lhs_from = (value << pow);
            else
            {
                value = -value;
                *lhs_from = -(value << pow);
            }
        }
        else
        {
            if(value >= 0)
                *lhs_from = (value >> pow);
            else
            {
                value = -value;
                *lhs_from = -(value >> pow);
            }
        }
    }
}
//---------------------------------------------------------------------------
// perform x <- x/2^pow
template<typename iterator1,typename value_type>
void divide_pow_constant(iterator1 lhs_from,iterator1 lhs_to,value_type pow)
{
    typename std::iterator_traits<iterator1>::value_type value;
    if(pow == 0)
        return;
    if(pow > 0)
    {
        for (; lhs_from != lhs_to; ++lhs_from)
        {
            value = *lhs_from;
            if(value >= 0)
                *lhs_from = (value >> pow);
            else
            {
                value = -value;
                *lhs_from = -(value >> pow);
            }
        }
    }
    else
    {
        pow = -pow;
        for (; lhs_from != lhs_to; ++lhs_from)
        {
            value = *lhs_from;
            if(value >= 0)
                *lhs_from = (value << pow);
            else
            {
                value = -value;
                *lhs_from = -(value << pow);
            }
        }
    }
}

//---------------------------------------------------------------------------
template<typename ValueType,int dimension>
ValueType max_abs_value(const basic_image<ValueType,dimension>& image)
{
    ValueType max_value = 0;
    typename basic_image<float,dimension>::const_iterator from = image.begin();
    typename basic_image<float,dimension>::const_iterator to = image.end();
    for (; from != to; ++from)
    {
        float value = *from;
        if (value > max_value)
            max_value = value;
        else if (-value > max_value)
            max_value = -value;
    }
    return max_value;
}

template<typename InputIter,typename OutputIter>
void normalize(InputIter from,InputIter to,OutputIter out,float upper_limit = 255.0)
{
    typedef typename std::iterator_traits<InputIter>::value_type value_type;
    std::pair<value_type,value_type> min_max(min_max_value(from,to));
    value_type range = min_max.second-min_max.first;
    if(range == 0)
        return;
    upper_limit /= range;
    for(;from != to;++from,++out)
        *out = (*from-min_max.first)*upper_limit;
}

template<typename InputIter,typename OutputIter,typename value_type>
void upper_threshold(InputIter from,InputIter to,OutputIter out,value_type upper)
{
    for(;from != to;++from,++out)
        *out = std::min<value_type>(*from,upper);
}
template<typename InputIter,typename value_type>
void upper_threshold(InputIter from,InputIter to,value_type upper)
{
    for(;from != to;++from)
        *from = std::min<value_type>(*from,upper);
}
template<typename InputIter,typename OutputIter,typename value_type>
void lower_threshold(InputIter from,InputIter to,OutputIter out,value_type lower)
{
    for(;from != to;++from,++out)
        *out = std::max<value_type>(*from,lower);
}
template<typename InputIter,typename value_type>
void lower_threshold(InputIter from,InputIter to,value_type lower)
{
    for(;from != to;++from)
        *from = std::max<value_type>(*from,lower);
}

template<typename InputIter,typename OutputIter,typename value_type>
void upper_lower_threshold(InputIter from,InputIter to,OutputIter out,value_type lower,value_type upper)
{
    for(;from != to;++from,++out)
        *out = std::min<value_type>(std::max<value_type>(*from,lower),upper);
}

template<typename ImageType,typename value_type>
void upper_lower_threshold(ImageType& data,value_type lower,value_type upper)
{
    typename ImageType::iterator from = data.begin();
    typename ImageType::iterator to = data.end();
    for(;from != to;++from)
        *from = std::min<value_type>(std::max<value_type>(*from,lower),upper);
}

template<typename ImageType>
void normalize(ImageType& image,float upper_limit = 255)
{
    multiply_constant(image.begin(),image.end(),upper_limit/(*std::max_element(image.begin(),image.end())));
}

template<typename ImageType1,typename ImageType2>
void normalize(const ImageType1& image1,ImageType2& image2,float upper_limit = 255.0)
{
    image2.resize(image1.geometry());
    normalize(image1.begin(),image1.end(),image2.begin(),upper_limit);
}


}
#endif
