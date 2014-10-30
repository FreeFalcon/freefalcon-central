#ifndef VU_TEMPLATES_H
#define VU_TEMPLATES_H
#include <cISO646>

/** @file vu_templates.h templates for VU. */

/** smartpointer to vuentity pointers. Upon creation, will ref entity. On destruction, will deref it */
template<class E>
class VuBin
{
public:
    /** creates smartpointer to entity. Will ref it. Also default constructor */
    explicit VuBin(E *e = NULL) : e(e)
    {
        VuReferenceEntity(e);
    }
    /** copy constructor, refs it too */
    VuBin(const VuBin &rhs) : e(rhs.e)
    {
        VuReferenceEntity(e);
    }
    /** derefs entity pointed to */
    ~VuBin()
    {
        //VuDeReferenceEntity(e);
    }

    /** assignment using vubin */
    const VuBin &operator=(const VuBin &rhs)
    {
        reset(rhs.e);
        return *this;
    }

    /** equality test */
    bool operator==(const VuBin &rhs) const
    {
        return e == rhs.e;
    }
    /** different */
    bool operator not_eq (const VuBin &rhs)const
    {
        return not operator==(rhs);
    }

    // NULL tests
    bool operator not () const
    {
        return e == NULL;
    }
    operator bool() const
    {
        return e not_eq NULL;
    }

    /** returns the dumb pointer */
    E *operator*() const
    {
        return e;
    }

    /** returns the dumb pointer */
    E *operator->() const
    {
        return e;
    }

    /** gets the raw pointer */
    E *get() const
    {
        return e;
    }

    /** sets raw pointer */
    void reset(E *newe = NULL)
    {
        if (newe == e)
        {
            return;
        }

        VuReferenceEntity(newe); // ref new pointer
        VuDeReferenceEntity(e);  // unref old pointer
        e = newe;                // get new pointer
    }

private:
    /** the entity pointer */
    E *e;
};
/** equality between * and a VuBin */
template <class E> bool operator==(const void* le, const VuBin<E> &re)
{
    return ((void*)re.get()) == le;
}
/** difference between void * and a VuBin */
template <class E> bool operator not_eq (const void* le, const VuBin<E> &re)
{
    return ((void*)re.get()) not_eq le;
}


#endif

