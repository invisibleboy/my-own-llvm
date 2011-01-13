-- This file is generated by SWIG. Do *not* modify by hand.
--

with Interfaces.C.Extensions;


package LLVM_Target is

   -- LLVMOpaqueTargetData
   --
   type LLVMOpaqueTargetData is new
     Interfaces.C.Extensions.opaque_structure_def;

   type LLVMOpaqueTargetData_array is
     array (Interfaces.C.size_t range <>)
            of aliased LLVM_Target.LLVMOpaqueTargetData;

   type LLVMOpaqueTargetData_view is access all
     LLVM_Target.LLVMOpaqueTargetData;

   -- LLVMTargetDataRef
   --
   type LLVMTargetDataRef is access all LLVM_Target.LLVMOpaqueTargetData;

   type LLVMTargetDataRef_array is
     array (Interfaces.C.size_t range <>)
            of aliased LLVM_Target.LLVMTargetDataRef;

   type LLVMTargetDataRef_view is access all LLVM_Target.LLVMTargetDataRef;

   -- LLVMStructLayout
   --
   type LLVMStructLayout is new Interfaces.C.Extensions.opaque_structure_def;

   type LLVMStructLayout_array is
     array (Interfaces.C.size_t range <>)
            of aliased LLVM_Target.LLVMStructLayout;

   type LLVMStructLayout_view is access all LLVM_Target.LLVMStructLayout;

   -- LLVMStructLayoutRef
   --
   type LLVMStructLayoutRef is access all LLVM_Target.LLVMStructLayout;

   type LLVMStructLayoutRef_array is
     array (Interfaces.C.size_t range <>)
            of aliased LLVM_Target.LLVMStructLayoutRef;

   type LLVMStructLayoutRef_view is access all LLVM_Target.LLVMStructLayoutRef;

   -- TargetData
   --
   type TargetData is new Interfaces.C.Extensions.incomplete_class_def;

   type TargetData_array is
     array (Interfaces.C.size_t range <>)
            of aliased LLVM_Target.TargetData;

   type TargetData_view is access all LLVM_Target.TargetData;

   -- LLVMByteOrdering
   --
   type LLVMByteOrdering is new Interfaces.C.int;

   type LLVMByteOrdering_array is
     array (Interfaces.C.size_t range <>)
            of aliased LLVM_Target.LLVMByteOrdering;

   type LLVMByteOrdering_view is access all LLVM_Target.LLVMByteOrdering;


end LLVM_Target;