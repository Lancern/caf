; ModuleID = 'ctor-wrapper.bc'
source_filename = "ctor-wrapper.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%class.Point2d = type { i32, i32 }
%class.Point3d = type { i32, i32, i32 }

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @_ZNK7Point2d1xEv(%class.Point2d* %this) #0 align 2 {
entry:
  %this.addr = alloca %class.Point2d*, align 8
  store %class.Point2d* %this, %class.Point2d** %this.addr, align 8
  %this1 = load %class.Point2d*, %class.Point2d** %this.addr, align 8
  %_x = getelementptr inbounds %class.Point2d, %class.Point2d* %this1, i32 0, i32 0
  %0 = load i32, i32* %_x, align 4
  ret i32 %0
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @_ZNK7Point2d1yEv(%class.Point2d* %this) #0 align 2 {
entry:
  %this.addr = alloca %class.Point2d*, align 8
  store %class.Point2d* %this, %class.Point2d** %this.addr, align 8
  %this1 = load %class.Point2d*, %class.Point2d** %this.addr, align 8
  %_y = getelementptr inbounds %class.Point2d, %class.Point2d* %this1, i32 0, i32 1
  %0 = load i32, i32* %_y, align 4
  ret i32 %0
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @_ZNK7Point3d1xEv(%class.Point3d* %this) #0 align 2 {
entry:
  %this.addr = alloca %class.Point3d*, align 8
  store %class.Point3d* %this, %class.Point3d** %this.addr, align 8
  %this1 = load %class.Point3d*, %class.Point3d** %this.addr, align 8
  %_x = getelementptr inbounds %class.Point3d, %class.Point3d* %this1, i32 0, i32 0
  %0 = load i32, i32* %_x, align 4
  ret i32 %0
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @_ZNK7Point3d1yEv(%class.Point3d* %this) #0 align 2 {
entry:
  %this.addr = alloca %class.Point3d*, align 8
  store %class.Point3d* %this, %class.Point3d** %this.addr, align 8
  %this1 = load %class.Point3d*, %class.Point3d** %this.addr, align 8
  %_y = getelementptr inbounds %class.Point3d, %class.Point3d* %this1, i32 0, i32 1
  %0 = load i32, i32* %_y, align 4
  ret i32 %0
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @_ZNK7Point3d1zEv(%class.Point3d* %this) #0 align 2 {
entry:
  %this.addr = alloca %class.Point3d*, align 8
  store %class.Point3d* %this, %class.Point3d** %this.addr, align 8
  %this1 = load %class.Point3d*, %class.Point3d** %this.addr, align 8
  %_z = getelementptr inbounds %class.Point3d, %class.Point3d* %this1, i32 0, i32 2
  %0 = load i32, i32* %_z, align 4
  ret i32 %0
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 7.1.0 "}
