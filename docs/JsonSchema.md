# JsonSchema

This documentation illustrates the schema of the JSON representation of CAF metadata in a top-to-down way.

## `CAFStore`

`CAFStore` is the top-level container of all metadata. Its JSON representation corresponds to the following TypeScript description:

```typescript
interface CAFStore {
    types: Type[];
    apis: Function[];
}
```

### `CAFStoreRef`

`CAFStoreRef` is a thin smart pointer type that points to a type or an API function owned by a `CAFStore` object. `CAFStoreRef` defines a generic parameter (template type parameter in C++) that can be either `Type`s or `Function` indicating the type of the object pointed to by this `CAFStoreRef` pointer. The JSON representation of a `CAFStoreRef` is a single integer indicating the index of the slot inside the `CAFStore` object that contains the pointed-to object. The following TypeScript snippet describes the facade of a `CAFStoreRef` when serialized to JSON representation:

```typescript
type CAFStoreRef<T> = number;
```

## `Type`

`Type` is an abstract base class of four concrete classes: `BitsType`, `PointerType`, `ArrayType` and `StructType`. The JSON representation of these classes corresponds to the following TypeScript description:

```typescript
interface Type {
    kind: number;
    id: number;
}

interface BitsType {
    name: string;
    size: number;
}

interface PointerType {
    pointee: CAFStoreRef<Type>;
}

interface ArrayType {
    size: number;
    element: CAFStoreRef<Type>;
}

interface StructType {
    name: string;
    ctors: Constructor[];
}
```

The `Type.kind` field indicates the kind of the type. If the type is a bits type, then `Type.kind` is 0; if the type is a pointer type, then `Type.kind` is 1; if the type is an array type, then `Type.kind` is 2; if the type is a struct type, then `Type.kind` is 3.

## `Function`

`Function` represents an API function. Its JSON representation corresponds to the following TypeScript description:

```typescript
interface Function {
    id: number;
    name: string;
    signature: FunctionSignature;
}
```

### `FunctionSignature`

`FunctionSignature` represents the signature of a function-like object (function, constructr, etc.). Its JSON representation corresponds to the following TypeScript description:

```typescript
interface FunctionSignature {
    ret: CAFStoreRef<Type>;
    args: CAFStoreRef<Type>[];
}
```

## `Constructor`

`Constructor` represents a constructor of some struct type. Its JSON representation corresponds to the following TypeScript description:

```typescript
interface Constructor {
    id: number;
    signature: FunctionSignature;
}
```
