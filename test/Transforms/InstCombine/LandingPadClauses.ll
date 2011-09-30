; RUN: opt < %s -instcombine -S | FileCheck %s

@T1 = external constant i32
@T2 = external constant i32
@T3 = external constant i32

declare i32 @generic_personality(i32, i64, i8*, i8*)
declare i32 @__gxx_personality_v0(i32, i64, i8*, i8*)

declare void @bar()

define void @foo_generic() {
; CHECK: @foo_generic
  invoke void @bar()
    to label %cont.a unwind label %lpad.a
cont.a:
  invoke void @bar()
    to label %cont.b unwind label %lpad.b
cont.b:
  invoke void @bar()
    to label %cont.c unwind label %lpad.c
cont.c:
  invoke void @bar()
    to label %cont.d unwind label %lpad.d
cont.d:
  invoke void @bar()
    to label %cont.e unwind label %lpad.e
cont.e:
  invoke void @bar()
    to label %cont.f unwind label %lpad.f
cont.f:
  invoke void @bar()
    to label %cont.g unwind label %lpad.g
cont.g:
  invoke void @bar()
    to label %cont.h unwind label %lpad.h
cont.h:
  ret void

lpad.a:
  %a = landingpad { i8*, i32 } personality i32 (i32, i64, i8*, i8*)* @generic_personality
          catch i32* @T1
          catch i32* @T2
          catch i32* @T1
          catch i32* @T2
  unreachable
; CHECK: %a = landingpad
; CHECK-NEXT: @T1
; CHECK-NEXT: @T2
; CHECK-NEXT: unreachable

lpad.b:
  %b = landingpad { i8*, i32 } personality i32 (i32, i64, i8*, i8*)* @generic_personality
          filter [0 x i32*] zeroinitializer
          catch i32* @T1
  unreachable
; CHECK: %b = landingpad
; CHECK-NEXT: filter
; CHECK-NEXT: unreachable

lpad.c:
  %c = landingpad { i8*, i32 } personality i32 (i32, i64, i8*, i8*)* @generic_personality
          catch i32* @T1
          filter [1 x i32*] [i32* @T1]
          catch i32* @T2
  unreachable
; CHECK: %c = landingpad
; CHECK-NEXT: @T1
; CHECK-NEXT: filter [0 x i32*]
; CHECK-NEXT: unreachable

lpad.d:
  %d = landingpad { i8*, i32 } personality i32 (i32, i64, i8*, i8*)* @generic_personality
          filter [3 x i32*] zeroinitializer
  unreachable
; CHECK: %d = landingpad
; CHECK-NEXT: filter [1 x i32*] zeroinitializer
; CHECK-NEXT: unreachable

lpad.e:
  %e = landingpad { i8*, i32 } personality i32 (i32, i64, i8*, i8*)* @generic_personality
          catch i32* @T1
          filter [3 x i32*] [i32* @T1, i32* @T2, i32* @T2]
  unreachable
; CHECK: %e = landingpad
; CHECK-NEXT: @T1
; CHECK-NEXT: filter [1 x i32*] [i32* @T2]
; CHECK-NEXT: unreachable

lpad.f:
  %f = landingpad { i8*, i32 } personality i32 (i32, i64, i8*, i8*)* @generic_personality
          filter [2 x i32*] [i32* @T2, i32* @T1]
          filter [1 x i32*] [i32* @T1]
  unreachable
; CHECK: %f = landingpad
; CHECK-NEXT: filter [1 x i32*] [i32* @T1]
; CHECK-NEXT: unreachable

lpad.g:
  %g = landingpad { i8*, i32 } personality i32 (i32, i64, i8*, i8*)* @generic_personality
          filter [1 x i32*] [i32* @T1]
          catch i32* @T3
          filter [2 x i32*] [i32* @T2, i32* @T1]
  unreachable
; CHECK: %g = landingpad
; CHECK-NEXT: filter [1 x i32*] [i32* @T1]
; CHECK-NEXT: catch i32* @T3
; CHECK-NEXT: unreachable

lpad.h:
  %h = landingpad { i8*, i32 } personality i32 (i32, i64, i8*, i8*)* @generic_personality
          filter [2 x i32*] [i32* @T1, i32* null]
          filter [1 x i32*] zeroinitializer
  unreachable
; CHECK: %h = landingpad
; CHECK-NEXT: filter [1 x i32*] zeroinitializer
; CHECK-NEXT: unreachable
}

define void @foo_cxx() {
; CHECK: @foo_cxx
  invoke void @bar()
    to label %cont.a unwind label %lpad.a
cont.a:
  invoke void @bar()
    to label %cont.b unwind label %lpad.b
cont.b:
  invoke void @bar()
    to label %cont.c unwind label %lpad.c
cont.c:
  ret void

lpad.a:
  %a = landingpad { i8*, i32 } personality i32 (i32, i64, i8*, i8*)* @__gxx_personality_v0
          catch i32* null
          catch i32* @T1
  unreachable
; CHECK: %a = landingpad
; CHECK-NEXT: null
; CHECK-NEXT: unreachable

lpad.b:
  %b = landingpad { i8*, i32 } personality i32 (i32, i64, i8*, i8*)* @__gxx_personality_v0
          filter [1 x i32*] zeroinitializer
  unreachable
; CHECK: %b = landingpad
; CHECK-NEXT: cleanup
; CHECK-NEXT: unreachable

lpad.c:
  %c = landingpad { i8*, i32 } personality i32 (i32, i64, i8*, i8*)* @__gxx_personality_v0
          filter [2 x i32*] [i32* @T1, i32* null]
  unreachable
; CHECK: %c = landingpad
; CHECK-NEXT: cleanup
; CHECK-NEXT: unreachable
}