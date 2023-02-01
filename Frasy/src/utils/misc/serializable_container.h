#ifndef SERIALIZER__SERIALIZABLE_CONTAINER_H
#define SERIALIZER__SERIALIZABLE_CONTAINER_H

namespace Frasy
{
/**
 * A serializable container is a container that can be iterated over, and who's values are also
 * serializable.
 *
 * For a container to be serializable, the following requirements needs to be met:
 * <ul>
 *  <li>Member Types: <ul>
 *      <li>@c value_type: The type of the value. Must meet <a
 * href=https://en.cppreference.com/w/cpp/types/is_arithmetic>@c std::is_arithmetic</a>.</li>
 *      <li>@c iterator: Iterator to @c value_type (e.g. @c value_type* ).</li>
 *      <li>@c const_iterator: Constant iterator @c value_type (e.g. @c const @c value_type* ).</li>
 *      <li>@c size_type: Unsigned integer type (usually @c std::size_t ).</li>
 *  </ul></li>
 *  <li>Member Functions: <ul>
 *
 *  </ul></li>
 * </ul>
 * @tparam T
 */
template<typename T>
concept SerializableContainer = requires(T t) {
                                    typename T::value_type;

                                    typename T::iterator;
                                    typename T::const_iterator;
                                    {
                                        t.begin()
                                    } -> std::same_as<typename T::iterator>;
                                    {
                                        std::as_const(t).begin()
                                    } -> std::same_as<typename T::const_iterator>;

                                    {
                                        t.end()
                                    } -> std::same_as<typename T::iterator>;
                                    {
                                        std::as_const(t).end()
                                    } -> std::same_as<typename T::const_iterator>;

                                    {
                                        t.size()
                                    } -> std::same_as<typename T::size_type>;
                                    {
                                        std::as_const(t).size()
                                    } -> std::same_as<typename T::size_type>;
                                };
static_assert(SerializableContainer<std::string>);
static_assert(SerializableContainer<std::array<char, 256>>);
}    // namespace Frasy

#endif    // SERIALIZER__SERIALIZABLE_CONTAINER_H
