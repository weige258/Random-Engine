#pragma once

#include <concepts>
#include <iterator>
#include <vector>
#include <type_traits>
#include <stdexcept>

namespace RandEngine::Core::Math{

// 静态范围类
template <int Start, int End, int Step = 1>
struct Range final
{
    private:
    static constexpr int m_start = Start;   
    static constexpr int m_end = End;      
    static constexpr int m_step = Step;     
    
    public:

    static_assert(Step != 0, "Step cannot be zero");

    // 编译期计算元素个数（可复用成员，也可继续用模板参数）
    static constexpr size_t Size()
    {
        if constexpr (m_step > 0) // 改用成员step_，和模板参数Step等价
        {
            return (m_end > m_start) ? (m_end - m_start + m_step - 1) / m_step : 0;
        }
        else
        {
            return (m_end < m_start) ? (m_start - m_end - m_step - 1) / (-m_step) : 0;
        }
    };
    
    // 迭代器支持（迭代器内也可访问类的静态成员）
    struct Iterator
    {
        int current;
        using iterator_category = std::forward_iterator_tag;
        using value_type = int;
        using difference_type = std::ptrdiff_t;
        using pointer = const int*;
        using reference = const int&;

        constexpr int operator*() const { return current; }
        
        constexpr Iterator &operator++()
        {
            current += Range::m_step; 
            return *this;
        }

        constexpr bool operator!=(const Iterator &other) const
        {
            if constexpr (Range::m_step > 0) 
                return current < other.current;
            else
                return current > other.current;
        }

        constexpr bool operator==(const Iterator &other) const
        {
            return !(*this != other);
        }
    };
    
    constexpr Iterator begin() const { return Iterator{Range::m_start}; } // 改用成员start_
    constexpr Iterator end() const { return Iterator{Range::m_end}; }     // 改用成员end_

    // 索引访问（改用类成员）
    constexpr int operator[](const size_t &index) const
    {
        if (index >= static_cast<std::size_t>(Size()))
        {
            throw std::out_of_range("Range index out of bounds!");
        }
        return Range::m_start + static_cast<int>(index) * Range::m_step;
    }
    
    // 值访问（改用类成员）
    static constexpr std::array<int, Size()> Values() {
        return []<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::array<int, Size()>{ (Range::m_start + static_cast<int>(Is) * Range::m_step)... };
        }(std::make_index_sequence<Size()>{});
    }

    // 转换为vector（改用类成员）
    constexpr operator std::vector<int>() const {
        return []<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::vector<int>{ (Range::m_start + static_cast<int>(Is) * Range::m_step)... };
        }(std::make_index_sequence<Size()>{});
    }

    // 转换为list（改用类成员）
    operator std::list<int>() const {
        std::list<int> result;
        for (int val : *this) {
            result.push_back(val);
        }
        return result;
    }

    // 转换为array（改用类成员）
    constexpr operator std::array<int, Size()>() const {
        return []<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::array<int, Size()>{ (Range::m_start + static_cast<int>(Is) * Range::m_step)... };
        }(std::make_index_sequence<Size()>{});
    }   
};

}