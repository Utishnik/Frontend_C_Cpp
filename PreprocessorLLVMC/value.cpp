#include <assert.h>

using namespace std;

class LLVMContext {
public:
    LLVMContext();
    LLVMContext(const LLVMContext&) = delete;
    LLVMContext& operator=(const LLVMContext&) = delete;
    ~LLVMContext();

    // Pinned metadata names, which always have the same value.  This is a
    // compile-time performance optimization, not a correctness optimization.
    enum : unsigned {
#define LLVM_FIXED_MD_KIND(EnumID, Name, Value) EnumID = Value,
#undef LLVM_FIXED_MD_KIND
    };

    /// Known operand bundle tag IDs, which always have the same value.  All
    /// operand bundle tags that LLVM has special knowledge of are listed here.
    /// Additionally, this scheme allows LLVM to efficiently check for specific
    /// operand bundle tags without comparing strings. Keep this in sync with
    /// LLVMContext::LLVMContext().
    enum : unsigned {
        OB_deopt = 0,                  // "deopt"
        OB_funclet = 1,                // "funclet"
        OB_gc_transition = 2,          // "gc-transition"
        OB_cfguardtarget = 3,          // "cfguardtarget"
        OB_preallocated = 4,           // "preallocated"
        OB_gc_live = 5,                // "gc-live"
        OB_clang_arc_attachedcall = 6, // "clang.arc.attachedcall"
        OB_ptrauth = 7,                // "ptrauth"
        OB_kcfi = 8,                   // "kcfi"
        OB_convergencectrl = 9,        // "convergencectrl"
    };

};

class Type {
public:
    //===--------------------------------------------------------------------===//
    /// Definitions of all of the base types for the Type system.  Based on this
    /// value, you can cast to a class defined in DerivedTypes.h.
    /// Note: If you add an element to this, you need to add an element to the
    /// Type::getPrimitiveType function, or else things will break!
    /// Also update LLVMTypeKind and LLVMGetTypeKind () in the C binding.
    ///
    enum TypeID {
        // PrimitiveTypes
        HalfTyID = 0,  ///< 16-bit floating point type
        BFloatTyID,    ///< 16-bit floating point type (7-bit significand)
        FloatTyID,     ///< 32-bit floating point type
        DoubleTyID,    ///< 64-bit floating point type
        X86_FP80TyID,  ///< 80-bit floating point type (X87)
        FP128TyID,     ///< 128-bit floating point type (112-bit significand)
        PPC_FP128TyID, ///< 128-bit floating point type (two 64-bits, PowerPC)
        VoidTyID,      ///< type with no size
        LabelTyID,     ///< Labels
        MetadataTyID,  ///< Metadata
        X86_MMXTyID,   ///< MMX vectors (64 bits, X86 specific)
        X86_AMXTyID,   ///< AMX vectors (8192 bits, X86 specific)
        TokenTyID,     ///< Tokens

        // Derived types... see DerivedTypes.h file.
        IntegerTyID,        ///< Arbitrary bit width integers
        FunctionTyID,       ///< Functions
        PointerTyID,        ///< Pointers
        StructTyID,         ///< Structures
        ArrayTyID,          ///< Arrays
        FixedVectorTyID,    ///< Fixed width SIMD vector type
        ScalableVectorTyID, ///< Scalable SIMD vector type
        TypedPointerTyID,   ///< Typed pointer used by some GPU targets
        TargetExtTyID,      ///< Target extension type
    };

private:
    /// This refers to the LLVMContext in which this type was uniqued.
    LLVMContext& Context;

    TypeID   ID : 8;            // The current base type of this type.
    unsigned SubclassData : 24; // Space for subclasses to store data.
    // Note that this should be synchronized with
    // MAX_INT_BITS value in IntegerType class.

protected:
    friend class LLVMContextImpl;

    explicit Type(LLVMContext& C, TypeID tid)
        : Context(C), ID(tid), SubclassData(0) {}
    ~Type() = default;

    unsigned getSubclassData() const { return SubclassData; }

    void setSubclassData(unsigned val) {
        SubclassData = val;
        // Ensure we don't have any accidental truncation.
        assert(getSubclassData() == val && "Subclass data too large for field");
    }

    /// Keeps track of how many Type*'s there are in the ContainedTys list.
    unsigned NumContainedTys = 0;

    /// A pointer to the array of Types contained by this Type. For example, this
    /// includes the arguments of a function type, the elements of a structure,
    /// the pointee of a pointer, the element type of an array, etc. This pointer
    /// may be 0 for types that don't contain other types (Integer, Double,
    /// Float).
    Type* const* ContainedTys = nullptr;
};

class Use {
    class Value;

    private:
        Value* Val = nullptr;
        Use* Next = nullptr;
        Use** Prev = nullptr;

        Value* Get_Val() { return Val; }
};

class Value {
    Type* VTy;
    Use* UseList;
    unsigned char value_type;

    friend class ValueAsMetadata; // Allow access to IsUsedByMD.
    friend class ValueHandleBase;

    const unsigned char SubclassID;   // Subclass identifier (for isa/dyn_cast)
    unsigned char HasValueHandle : 1; // Has a ValueHandle pointing to this?

protected:
    /// Hold subclass data that can be dropped.
    ///
    /// This member is similar to SubclassData, however it is for holding
    /// information which may be used to aid optimization, but which may be
    /// cleared to zero without affecting conservative interpretation.
    unsigned char SubclassOptionalData : 7;

private:
    /// Hold arbitrary subclass data.
    ///
    /// This member is defined by this class, but is not used for anything.
    /// Subclasses can use it to hold whatever state they find useful.  This
    /// field is initialized to zero by the ctor.
    unsigned short SubclassData;

protected:
    /// The number of operands in the subclass.
    ///
    /// This member is defined by this class, but not used for anything.
    /// Subclasses can use it to store their number of operands, if they have
    /// any.
    ///
    /// This is stored here to save space in User on 64-bit hosts.  Since most
    /// instances of Value have operands, 32-bit hosts aren't significantly
    /// affected.
    ///
    /// Note, this should *NOT* be used directly by any class other than User.
    /// User uses this value to find the Use list.
    enum : unsigned { NumUserOperandsBits = 27 };
    unsigned NumUserOperands : NumUserOperandsBits;

    // Use the same type as the bitfield above so that MSVC will pack them.
    unsigned IsUsedByMD : 1;
    unsigned HasName : 1;
    unsigned HasMetadata : 1; // Has metadata attached to this?
    unsigned HasHungOffUses : 1;
    unsigned HasDescriptor : 1;

private:
    template <typename UseT> // UseT == 'Use' or 'const Use'
    class use_iterator_impl {
        friend class Value;

        UseT* U;

        explicit use_iterator_impl(UseT* u) : U(u) {}

    public:
         int iterator_category;
         Value value_type = *UseT;
         Value pointer = *value_type;
    };
};