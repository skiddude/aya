<script>
    let { transport } = $props();
</script>

<div class="flex h-full w-full flex-col justify-start gap-3 overflow-scroll p-4 pb-24">
    <div class="flex w-full items-center"><span class="text-3xl font-extrabold">Running Jobs</span><button class="ml-auto flex items-center rounded border border-neutral-300 bg-white px-3 py-1 text-sm text-neutral-600 shadow-sm hover:bg-gray-100"><i class="fa fa-regular fa-play fa-fw me-2"></i>Start</button></div>

    <div class="flex items-center rounded border border-neutral-400/75 bg-gray-50/90 px-3 py-4">
        <i class="fa fa-light fa-server fa-fw me-1 text-6xl text-neutral-600"></i>
        <div class="flex flex-col">
            <span class="text-xl font-bold text-neutral-900">127.0.0.1:53640</span>
            <span class="font-mono text-xs text-neutral-500">9f4e11e2-f13b-470f-b6cd-ada7d82cdf2f</span>
        </div>
        <!-- these go on the far right -->
        <div class="ml-auto flex gap-3">
            <button class="flex items-center rounded border border-neutral-300 bg-white px-3 py-1 text-sm text-neutral-600 shadow-sm hover:bg-gray-100">
                <i class="fa fa-regular fa-eye fa-fw me-2"></i>Inspect
            </button>
            <button class="flex items-center rounded border border-red-400 bg-red-500/10 px-3 py-1 text-sm text-red-600 shadow-sm hover:bg-red-500/20">
                <i class="fa fa-regular fa-xmark fa-fw me-2"></i>Stop Job
            </button>
        </div>
    </div>
    <div class="flex items-center rounded border border-neutral-400/75 bg-gray-50/90 px-3 py-4">
        <i class="fa fa-light fa-server fa-fw me-1 text-6xl text-neutral-600"></i>
        <div class="flex flex-col">
            <span class="text-xl font-bold text-neutral-900">Test Job</span>
            <span class="font-mono text-sm text-neutral-500">a27df45e-670e-4a6b-94fa-d65f9ce844f3</span>
        </div>
        <!-- these go on the far right -->
        <div class="ml-auto flex gap-3">
            <button class="flex items-center rounded border border-neutral-300 bg-white px-3 py-1 text-sm text-neutral-600 shadow-sm hover:bg-gray-100">
                <i class="fa fa-regular fa-eye fa-fw me-2"></i>Inspect
            </button>
            <button class="flex items-center rounded border border-red-400 bg-red-500/10 px-3 py-1 text-sm text-red-600 shadow-sm hover:bg-red-500/20">
                <i class="fa fa-regular fa-xmark fa-fw me-2"></i>Stop Job
            </button>
        </div>
    </div>

    <div class="flex items-center rounded border border-neutral-400/75 bg-gray-50/90 px-3 py-4">
        <i class="fa fa-light fa-server fa-fw me-1 text-6xl text-neutral-600"></i>
        <div class="flex flex-col">
            <span class="text-xl font-bold text-neutral-900">yrdts</span>
            <span class="font-mono text-sm text-neutral-500">72110ea2-de5e-4c82-b203-05b62cac9aa7</span>
        </div>
        <!-- these go on the far right -->
        <div class="ml-auto flex gap-3">
            <button class="flex items-center rounded border border-neutral-300 bg-white px-3 py-1 text-sm text-neutral-600 shadow-sm hover:bg-gray-100">
                <i class="fa fa-regular fa-eye fa-fw me-2"></i>Inspect
            </button>
            <button class="flex items-center rounded border border-red-400 bg-red-500/10 px-3 py-1 text-sm text-red-600 shadow-sm hover:bg-red-500/20">
                <i class="fa fa-regular fa-xmark fa-fw me-2"></i>Stop Job
            </button>
        </div>
    </div>
</div>
