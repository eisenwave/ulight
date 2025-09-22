class Base { }
interface Interface { }
class C extends Base implements Interface {
    b: boolean
    constructor(b: any) {
        super()
        this.b = b
    }
    private x?: any
    protected y?: any
    public z?: any
}
